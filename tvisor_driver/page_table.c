#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>

uint64_t vaddr2paddr(uint64_t vaddr)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	uint64_t paddr = 0;
	uint64_t page_addr = 0;
	uint64_t page_offset = 0;

	pgd = pgd_offset(current->mm, vaddr);
	// pr_info("pgd_val = 0x%lx\n", pgd_val(*pgd));
	// pr_info("pgd_index = %llu\n", pgd_index(vaddr));
	if (pgd_none(*pgd)) {
		pr_alert("not mapped in pgd\n");
		return -1;
	}

	p4d = p4d_offset(pgd, vaddr);
	// pr_info("p4d_val = 0x%lx\n", p4d_val(*p4d));
	if (p4d_none(*p4d)) {
		pr_alert("not mapped in p4d\n");
		return -1;
	}

	pud = pud_offset(p4d, vaddr);
	// pr_info("pud_val = 0x%lx\n", pud_val(*pud));
	if (pud_none(*pud)) {
		pr_alert("not mapped in pud\n");
		return -1;
	}

	pmd = pmd_offset(pud, vaddr);
	// pr_info("pmd_val = 0x%lx\n", pmd_val(*pmd));
	// pr_info("pmd_index = %lu\n", pmd_index(vaddr));
	if (pmd_none(*pmd)) {
		pr_alert("not mapped in pmd\n");
		return -1;
	}

	pte = pte_offset_kernel(pmd, vaddr);
	// pr_info("pte_val = 0x%lx\n", pte_val(*pte));
	// pr_info("pte_index = %lu\n", pte_index(vaddr));
	if (pte_none(*pte)) {
		pr_alert("not mapped in pte\n");
		return -1;
	}

	page_addr = pte_val(*pte) & PAGE_MASK;
	page_offset = vaddr & ~PAGE_MASK;
	paddr = page_addr | page_offset;

	// pr_info("page_addr = 0x%llx, page_offset = 0x%llx\n", page_addr, page_offset);
	// pr_info("vaddr = 0x%llx, paddr = 0x%llx\n", vaddr, paddr);

	return paddr;
}