#include <linux/mm.h>
#include <linux/slab.h>

#include "cpu.h"
#include "ept.h"
#include "util.h"

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

static void free_ept_pt(ept_pte_t *pt)
{
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

static void free_ept_pd(ept_pde_t *pd)
{
	size_t i;
	for (i = 0; i < 512; i++) {
		u64 pt_phys = pd[i].ept_pt_address;
		if (pt_phys == 0xfffffffff) {
			continue;
		} else {
			ept_pte_t *pt = __va(pt_phys * 0x1000);
			free_ept_pt(pt);
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

static void free_ept_pdpt(ept_pdpte_t *pdpt)
{
	size_t i;
	for (i = 0; i < 512; i++) {
		u64 pd_phys = pdpt[i].ept_pd_address;
		if (pd_phys == 0xfffffffff) {
			continue;
		} else {
			ept_pde_t *pd = __va(pd_phys * 0x1000);
			free_ept_pd(pd);
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

static void free_ept_pml4(ept_pml4e_t *pml4)
{
	size_t i;
	for (i = 0; i < 512; i++) {
		u64 pdpt_phys = pml4[i].ept_pdpt_address;
		if (pdpt_phys == 0xfffffffff) {
			continue;
		} else {
			ept_pdpte_t *pdpt = __va(pdpt_phys * 0x1000);
			free_ept_pdpt(pdpt);
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

// create EPT
// request physical memory size is `mem_mib`(MiB)
ept_pointer_t *create_ept(u64 mem_mib)
{
	cpuid_t cpuid = get_cpuid(0x80000008);
	size_t phys_addr_bits = (size_t)(cpuid.eax & 0xff);

	// calc number of page tables
	size_t mem_kib = mem_mib * 0x1000;
	size_t need_ept_pt = (mem_kib + 512 - 1) / 512;
	size_t need_ept_pd = (need_ept_pt + 512 - 1) / 512;
	size_t need_ept_pdpt = (need_ept_pd + 512 - 1) / 512;

	// TODO: implementation

	ept_pointer_t *eptp = alloc_ept_pointer();
	if (eptp == NULL) {
		return NULL;
	}

	ept_pml4e_t *pml4 = alloc_ept_pml4();
	if (pml4 == NULL) {
		free_ept_pointer(eptp);
		return NULL;
	}
	eptp->fields.ept_pml4_table_address = __pa(pml4) / 0x1000;

	size_t i, j, k, l;

	for (i = 0; i < need_ept_pdpt; i++) {
		ept_pdpte_t *pdpt = alloc_ept_pdpt();
		if (pdpt == NULL) {
			free_ept_pointer(eptp);
			size_t j;
			for (j = 0; j < i; j++) {
				u64 pdpt_phys = pml4[j].ept_pdpt_address;
				ept_pdpte_t *free_pdpt = __va(pdpt_phys);
				free_ept_pdpt(free_pdpt);
			}
			free_ept_pml4(pml4);
			return NULL:
		} else {
		}
	}

	for (i = 0; i < need_ept_pd; i++) {
		ept_pde_t *pd = alloc_ept_pd();
		if (pd == NULL) {
		}
	}

	return eptp;
}

ept_pointer_t *init_ept_pointer(void)
{
	struct page *page;

	// alloc EPT Pointer
	page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		return NULL;
	}
	ept_pointer_t *eptp = (ept_pointer_t *)page_address(page);
	memset(eptp, 0, PAGE_SIZE);

	// alloc EPT PML4
	page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		__free_page(virt_to_page(eptp));
		return NULL;
	}
	ept_pml4e_t *ept_pml4 = (ept_pml4e_t *)page_address(page);
	memset(ept_pml4, 0, PAGE_SIZE);

	// alloc EPT PDP Table
	page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		__free_page(virt_to_page(eptp));
		__free_page(virt_to_page(ept_pml4));
		return NULL;
	}
	ept_pdpte_t *ept_pdpt = (ept_pdpte_t *)page_address(page);
	memset(ept_pdpt, 0, PAGE_SIZE);

	// alloc EPT PD
	page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		__free_page(virt_to_page(eptp));
		__free_page(virt_to_page(ept_pml4));
		__free_page(virt_to_page(ept_pdpt));
		return NULL;
	}
	ept_pde_t *ept_pd = (ept_pde_t *)page_address(page);
	memset(ept_pd, 0, PAGE_SIZE);

	// alloc EPT PT
	page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		__free_page(virt_to_page(eptp));
		__free_page(virt_to_page(ept_pml4));
		__free_page(virt_to_page(ept_pdpt));
		__free_page(virt_to_page(ept_pd));
		return NULL;
	}
	ept_pte_t *ept_pt = (ept_pte_t *)page_address(page);
	memset(ept_pt, 0, PAGE_SIZE);

	/* test setup */

	// setup PT by allocating two pages continuously
	const int order = 2;
	u64 num_pages = power(2, order);
	page = alloc_pages(GFP_KERNEL, order);
	if (page == NULL) {
		return NULL;
	}
	u64 *guest_memory = page_address(page);
	memset(guest_memory, 0, PAGE_SIZE * num_pages);
	size_t i;
	for (i = 0; i < num_pages; i++) {
		ept_pt[i].fields.accessed = 0;
		ept_pt[i].fields.dirty = 0;
		ept_pt[i].fields.memory_type = 6;
		ept_pt[i].fields.execute = 1;
		ept_pt[i].fields.execute_for_user_mode = 0;
		ept_pt[i].fields.ignore_pat = 0;
		ept_pt[i].fields.page_address =
			__pa(guest_memory + (i * PAGE_SIZE)) / PAGE_SIZE;
		ept_pt[i].fields.read = 1;
		ept_pt[i].fields.suppress_ve = 0;
		ept_pt[i].fields.write = 1;
	}
	// setup PDE
	ept_pd->fields.accessed = 0;
	ept_pd->fields.execute = 1;
	ept_pd->fields.execute_for_user_mode = 0;
	ept_pd->fields.ignored1 = 0;
	ept_pd->fields.ignored2 = 0;
	ept_pd->fields.ignored3 = 0;
	ept_pd->fields.ept_pt_address = (__pa(ept_pt) / PAGE_SIZE);
	ept_pd->fields.read = 1;
	ept_pd->fields.reserved1 = 0;
	ept_pd->fields.reserved2 = 0;
	ept_pd->fields.write = 1;
	// setup PDPTE
	ept_pdpt->fields.accessed = 0;
	ept_pdpt->fields.execute = 1;
	ept_pdpt->fields.execute_for_user_mode = 0;
	ept_pdpt->fields.ignored1 = 0;
	ept_pdpt->fields.ignored2 = 0;
	ept_pdpt->fields.ignored3 = 0;
	ept_pdpt->fields.ept_pd_address = (__pa(ept_pd) / PAGE_SIZE);
	ept_pdpt->fields.read = 1;
	ept_pdpt->fields.reserved1 = 0;
	ept_pdpt->fields.reserved2 = 0;
	ept_pdpt->fields.write = 1;
	// setup PML4E
	ept_pml4->fields.accessed = 0;
	ept_pml4->fields.execute = 1;
	ept_pml4->fields.execute_for_user_mode = 0;
	ept_pml4->fields.ignored1 = 0;
	ept_pml4->fields.ignored2 = 0;
	ept_pml4->fields.ignored3 = 0;
	ept_pml4->fields.ept_pdpt_address = (__pa(ept_pdpt) / PAGE_SIZE);
	ept_pml4->fields.read = 1;
	ept_pml4->fields.reserved1 = 0;
	ept_pml4->fields.reserved2 = 0;
	ept_pml4->fields.write = 1;
	// setup EPT Pointer
	eptp->fields.dirty_and_access_enabled = 1;
	eptp->fields.memory_type = 6; // 6 => Write-Back
	eptp->fields.page_walk_length = 3; // 4(tables walked) - 1 = 3
	eptp->fields.ept_pml4_table_address = (__pa(ept_pml4) / PAGE_SIZE);
	eptp->fields.reserved1 = 0;
	eptp->fields.reserved2 = 0;

	pr_info("tvisor: EPT Pointer allocated at %p\n", eptp);

	return eptp;
}