#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <asm/io.h> /* Needed for virt_to_phys */

#include "vm.h"
#include "cpu.h"
#include "page_table.h"

int allocate_vmxon_region(vm_state_t *vm_state)
{
	struct page *pg;
	uint64_t *virt;
	uint64_t phys;
	int status;
	ia32_vmx_basic_msr_t basic;

	pg = alloc_page(GFP_ATOMIC | __GFP_NORETRY);
	if (pg == NULL) {
		pr_alert("alloc_page failed\n");
		return 0;
	}

	virt = (uint64_t *)page_address(pg);
	if (virt == NULL) {
		pr_alert("virtual addredd is not found\n");
		return 0;
	}
	activate_page(pg);
	memset(virt, 0, 0x1000);

	phys = virt_to_phys(virt);
	basic.all = read_msr(0x480); /* MSR_IA32_VMX_BASIC */

	pr_info("MSR_IA32_VMX_BASIC Rev ID: %x\n", basic.fields.revision_id);
	*(uint32_t *)virt = basic.fields.revision_id;

	status = vmx_on(phys);
	if (status) {
		pr_alert("vmx_on failed with status %d\n", status);
		return 0;
	}

	vm_state->vmxon_region = phys;

	/* test */
	pr_info("virt = %llx, phys = %llx\n", (uint64_t)virt, phys);
	//free_page((uint64_t)virt);

	return 1;
}