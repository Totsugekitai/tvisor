#include <linux/cpumask.h> /* Needed for cpumask_* */
#include <linux/smp.h> /* Needed for on_each_cpu_mask */
#include <linux/slab.h> /* Needed for kmalloc */
#include <linux/mm.h> /* Needed for page_address */

#include "vm.h"

vm_state_t vm;

static void __launch_vm(void *info)
{
	vm_state_t *vm = (vm_state_t *)info;
}

void launch_vm(int cpu, vm_state_t *vm)
{
	struct cpumask mask;
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	on_each_cpu_mask(&mask, __launch_vm, vm, 1);
}

vm_state_t *create_vm(void)
{
	vm_state_t *vm = kmalloc(sizeof(vm_state_t), GFP_KERNEL);
	if (vm == NULL) {
		return NULL;
	}

	vmxon_region_t *vmxon_region = alloc_vmxon_region();
	if (vmxon_region == NULL) {
		kfree(vm);
		return NULL;
	}

	vmcs_t *vmcs_region = alloc_vmcs_region();
	if (vmcs_region == NULL) {
		kfree(vm);
		free_vmxon_region(vmxon_region);
		return NULL;
	}

	ept_pointer_t *ept_pointer = init_ept_pointer();
	if (ept_pointer == NULL) {
		kfree(vm);
		free_vmxon_region(vmxon_region);
		free_vmcs_region(vmcs_region);
		return NULL;
	}

	const int order = 3;
	struct page *vmm_stack_pages = alloc_pages(GFP_KERNEL, order);
	if (vmm_stack_pages == NULL) {
		kfree(vm);
		free_vmxon_region(vmxon_region);
		free_vmcs_region(vmcs_region);
		// TODO: free ept_pointer
		return NULL;
	}
	u64 *vmm_stack = (u64 *)page_address(vmm_stack_pages);

	vm->vmxon_region = vmxon_region;

	return vm;
}