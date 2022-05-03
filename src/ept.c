#include <linux/mm.h>
#include <linux/slab.h>

#include "cpu.h"
#include "ept.h"
#include "util.h"

static void *alloc_ept_page(void)
{
	struct page *page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		return NULL;
	}
	void *page_va = page_address(page);
	memset(page_va, 0, 0x1000);
	return page_va;
}

static void free_ept_page(void *page_va)
{
	__free_page(virt_to_page(page_va));
}

static ept_pte_t *alloc_ept_pt(void)
{
	struct page *page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		return NULL;
	}
	ept_pte_t *pt = (ept_pte_t *)page_address(page);
	memset(pt, 0, 0x1000);
	return pt;
}

static void free_ept_pt_recursive(ept_pte_t *pt)
{
	size_t i;
	for (i = 0; i < 512; i++) {
		u64 pg_ign1 = pt[i].fields.ignored1;
		if (pg_ign1 == 0) {
			continue;
		} else {
			u64 pg_phys = pt[i].fields.page_address;
			void *pg = __va(pg_phys * 0x1000);
			//pr_debug("tvisor: free pg %p\n", pg);
			free_ept_page(pg);
		}
	}
	__free_page(virt_to_page(pt));
}

static ept_pde_t *alloc_ept_pd(void)
{
	struct page *page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		return NULL;
	}
	ept_pde_t *pd = (ept_pde_t *)page_address(page);
	memset(pd, 0, 0x1000);
	return pd;
}

static void free_ept_pd_recursive(ept_pde_t *pd)
{
	size_t i;
	for (i = 0; i < 512; i++) {
		u64 pt_ign1 = pd[i].fields.ignored1;
		if (pt_ign1 == 0) {
			continue;
		} else {
			u64 pt_phys = pd[i].fields.ept_pt_address;
			ept_pte_t *pt = __va(pt_phys * 0x1000);
			//pr_debug("tvisor: free pt %p\n", pt);
			free_ept_pt_recursive(pt);
		}
	}
	__free_page(virt_to_page(pd));
}

static ept_pdpte_t *alloc_ept_pdpt(void)
{
	struct page *page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		return NULL;
	}
	ept_pdpte_t *pdpt = (ept_pdpte_t *)page_address(page);
	memset(pdpt, 0, 0x1000);
	return pdpt;
}

static void free_ept_pdpt_recursive(ept_pdpte_t *pdpt)
{
	size_t i;
	for (i = 0; i < 512; i++) {
		u64 pd_ign1 = pdpt[i].fields.ignored1;
		if (pd_ign1 == 0) {
			continue;
		} else {
			u64 pd_phys = pdpt[i].fields.ept_pd_address;
			ept_pde_t *pd = __va(pd_phys * 0x1000);
			free_ept_pd_recursive(pd);
			//pr_debug("tvisor: free pd %p\n", pd);
		}
	}
	__free_page(virt_to_page(pdpt));
}

static ept_pml4e_t *alloc_ept_pml4(void)
{
	struct page *page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		return NULL;
	}
	ept_pml4e_t *pml4 = (ept_pml4e_t *)page_address(page);
	memset(pml4, 0, 0x1000);
	return pml4;
}

static void free_ept_pml4_recursive(ept_pml4e_t *pml4)
{
	size_t i;
	for (i = 0; i < 512; i++) {
		u64 pdpt_ign1 = pml4[i].fields.ignored1;
		if (pdpt_ign1 == 0) {
			continue;
		} else {
			u64 pdpt_phys = pml4[i].fields.ept_pdpt_address;
			ept_pdpte_t *pdpt = __va(pdpt_phys * 0x1000);
			//pr_debug("tvisor: free pdpt %p\n", pdpt);
			free_ept_pdpt_recursive(pdpt);
		}
	}
	__free_page(virt_to_page(pml4));
}

static ept_pointer_t *alloc_ept_pointer(void)
{
	return (ept_pointer_t *)kmalloc(sizeof(ept_pointer_t), GFP_KERNEL);
}

static void free_ept_pointer(ept_pointer_t *eptp)
{
	kfree(eptp);
}

static void *alloc_page_rec_by_memsize(void)
{
	return alloc_ept_page();
}

static ept_pte_t *alloc_pt_rec_by_memsize(u64 size_kib)
{
	ept_pte_t *pt = alloc_ept_pt();
	if (pt == NULL) {
		return NULL;
	} else {
		size_t i;
		for (i = 0; i < size_kib; i++) {
			void *page = alloc_page_rec_by_memsize();
			if (page == NULL) {
				free_ept_pt_recursive(pt);
				return NULL;
			} else {
				pt[i].fields.page_address =
					(u64)(__pa(page) / 0x1000);
				pt[i].fields.accessed = 0;
				pt[i].fields.dirty = 0;
				pt[i].fields.memory_type = 6;
				pt[i].fields.read = 1;
				pt[i].fields.write = 1;
				pt[i].fields.execute = 1;
				pt[i].fields.execute_for_user_mode = 0;
				pt[i].fields.ignore_pat = 0;
				pt[i].fields.ignored1 = 1; // use as used flag
				pt[i].fields.ignored2 = 0;
				pt[i].fields.ignored3 = 0;
				pt[i].fields.ignored4 = 0;
				pt[i].fields.suppress_ve = 0;
			}
		}
	}
	return pt;
}

static ept_pde_t *alloc_pd_rec_by_memsize(u64 size_mib)
{
	size_t size_kib = size_mib * 0x1000;
	//size_t need_ept_pt = (size_kib + 512 - 1) / 512;

	const size_t max_kib_per_entry = 0x200;

	ept_pde_t *pd = alloc_ept_pd();
	if (pd == NULL) {
		return NULL;
	} else {
		size_t i, copy_kib = size_kib;
		//for (i = 0; i < need_ept_pt; i++) {
		for (i = 0; copy_kib > 0; i++) {
			size_t assign_kib = 0;
			if (copy_kib > max_kib_per_entry) {
				assign_kib = max_kib_per_entry;
				copy_kib -= max_kib_per_entry;
			} else {
				assign_kib = copy_kib;
				copy_kib -= assign_kib;
			}
			pr_debug("tvisor: alloc %lxKib memory\n", assign_kib);
			ept_pte_t *pt = alloc_pt_rec_by_memsize(assign_kib);
			if (pt == NULL) {
				free_ept_pd_recursive(pd);
				return NULL;
			}
			pd[i].fields.ept_pt_address = __pa(pt) / 0x1000;
			pd[i].fields.accessed = 0;
			pd[i].fields.read = 1;
			pd[i].fields.write = 1;
			pd[i].fields.execute = 1;
			pd[i].fields.execute_for_user_mode = 0;
			pd[i].fields.ignored1 = 1;
			pd[i].fields.ignored2 = 0;
			pd[i].fields.ignored3 = 0;
			pd[i].fields.reserved1 = 0;
			pd[i].fields.reserved2 = 0;
		}
	}
	return pd;
}

static ept_pdpte_t *alloc_pdpt_rec_by_memsize(u64 size_mib)
{
	//size_t size_kib = size_mib * 0x1000;
	//size_t need_ept_pt = (size_kib + 512 - 1) / 512;
	//size_t need_ept_pd = (need_ept_pt + 512 - 1) / 512;

	const size_t max_mib_per_entry = 0x40;

	ept_pdpte_t *pdpt = alloc_ept_pdpt();
	if (pdpt == NULL) {
		return NULL;
	} else {
		size_t i, copy_mib = size_mib;
		//for (i = 0; i < need_ept_pd; i++) {
		for (i = 0; copy_mib > 0; i++) {
			size_t assign_mib = 0;
			if (copy_mib > max_mib_per_entry) {
				assign_mib = max_mib_per_entry;
				copy_mib -= max_mib_per_entry;
			} else {
				assign_mib = copy_mib;
				copy_mib -= assign_mib;
			}
			pr_debug("tvisor: alloc %lxMib memory\n", assign_mib);
			ept_pde_t *pd = alloc_pd_rec_by_memsize(assign_mib);
			if (pd == NULL) {
				free_ept_pdpt_recursive(pdpt);
				return NULL;
			}
			pdpt[i].fields.ept_pd_address = __pa(pd) / 0x1000;
			pdpt[i].fields.accessed = 0;
			pdpt[i].fields.read = 1;
			pdpt[i].fields.write = 1;
			pdpt[i].fields.execute = 1;
			pdpt[i].fields.execute_for_user_mode = 0;
			pdpt[i].fields.ignored1 = 1; // use as used flag
			pdpt[i].fields.ignored2 = 0;
			pdpt[i].fields.ignored3 = 0;
			pdpt[i].fields.reserved1 = 0;
			pdpt[i].fields.reserved2 = 0;
		}
	}
	return pdpt;
}

static ept_pml4e_t *alloc_pml4_rec_by_memsize(u64 size_mib)
{
	//size_t size_kib = size_mib * 0x1000;
	//size_t need_ept_pt = (size_kib + 512 - 1) / 512;
	//size_t need_ept_pd = (need_ept_pt + 512 - 1) / 512;
	//size_t need_ept_pdpt = (need_ept_pd + 512 - 1) / 512;

	const size_t max_mib_per_entry = 0x8000;

	ept_pml4e_t *pml4 = alloc_ept_pml4();
	if (pml4 == NULL) {
		return NULL;
	} else {
		size_t i, copy_mib = size_mib;
		//for (i = 0; i < need_ept_pdpt; i++) {
		for (i = 0; copy_mib > 0; i++) {
			size_t assign_mib = 0;
			if (copy_mib > max_mib_per_entry) {
				assign_mib = max_mib_per_entry;
				copy_mib -= max_mib_per_entry;
			} else {
				assign_mib = copy_mib;
				copy_mib -= assign_mib;
			}
			pr_debug("tvisor: alloc %lxMib memory\n", assign_mib);
			ept_pdpte_t *pdpt =
				alloc_pdpt_rec_by_memsize(assign_mib);
			if (pdpt == NULL) {
				free_ept_pml4_recursive(pml4);
				return NULL;
			}
			pml4[i].fields.ept_pdpt_address = __pa(pdpt) / 0x1000;
			pml4[i].fields.accessed = 0;
			pml4[i].fields.read = 1;
			pml4[i].fields.write = 1;
			pml4[i].fields.execute = 1;
			pml4[i].fields.execute_for_user_mode = 0;
			pml4[i].fields.ignored1 = 1; // use as used flag
			pml4[i].fields.ignored2 = 0;
			pml4[i].fields.ignored3 = 0;
			pml4[i].fields.reserved1 = 0;
			pml4[i].fields.reserved2 = 0;
		}
	}
	return pml4;
}

static ept_pointer_t *alloc_ept_rec_by_memsize(u64 size_mib)
{
	ept_pointer_t *eptp = alloc_ept_pointer();
	if (eptp == NULL) {
		return NULL;
	} else {
		ept_pml4e_t *pml4 = alloc_pml4_rec_by_memsize(size_mib);
		if (pml4 == NULL) {
			free_ept_pointer(eptp);
			return NULL;
		}
		eptp->fields.ept_pml4_table_address = __pa(pml4) / 0x1000;
		eptp->fields.dirty_and_access_enabled = 1;
		eptp->fields.memory_type = 6; // 6 => Write-Back
		eptp->fields.page_walk_length = 3; // 4(tables walked) - 1 = 3
		eptp->fields.reserved1 = 0;
		eptp->fields.reserved2 = 0;
	}
	return eptp;
}

// create EPT
// request physical memory size is `size_mib`(MiB)
ept_pointer_t *create_ept_by_memsize(u64 size_mib)
{
	// TODO:
	// cpuid_t cpuid = get_cpuid(0x80000008);
	// size_t phys_addr_bits = (size_t)(cpuid.eax & 0xff);

	ept_pointer_t *eptp = alloc_ept_rec_by_memsize(size_mib);

	if (eptp == NULL) {
		pr_alert("tvisor: cannot allocate EPT\n");
	} else {
		pr_info("tvisor: EPT Pointer allocated at %p\n", eptp);
	}

	return eptp;
}

void free_ept(ept_pointer_t *eptp)
{
	u64 pml4_phys = eptp->fields.ept_pml4_table_address << 12;
	ept_pml4e_t *pml4 = __va(pml4_phys);
	free_ept_pointer(eptp);
	free_ept_pml4_recursive(pml4);
}
