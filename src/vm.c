#include <linux/cpumask.h> /* Needed for cpumask_* */
#include <linux/smp.h> /* Needed for on_each_cpu_mask */
#include <linux/slab.h> /* Needed for kmalloc */
#include <linux/mm.h> /* Needed for page_address */
#include <linux/printk.h> /* Needed for printk */

#include "vm.h"

struct tvisor_state {
	int is_virtualization_ready;
	int is_vmx_enabled;
};

extern struct tvisor_state TVISOR_STATE;

static void __launch_vm(void *info)
{
	vm_state_t *vm = (vm_state_t *)info;

	if (clear_vmcs_state(vm->vmcs_region)) {
		pr_info("tvisor: failed to clear vmcs state\n");
		return;
	}

	if (load_vmcs(vm->vmcs_region)) {
		pr_info("tvisor: failed to load vmcs\n");
		return;
	}

	setup_vmcs(vm->vmcs_region, vm->ept_pointer, vm->vmm_stack);

	save_vmxoff_state(&(vm->rsp), &(vm->rbp));

	int e = vmlaunch();
	pr_debug("tvisor: e=%x\n", e);

	u64 err = 0;
	vmread(VM_INSTRUCTION_ERROR, &err);
	vmxoff();
	TVISOR_STATE.is_vmx_enabled = 0;
	pr_debug("tvisor: vmlaunch error[%llx]\n", err);
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
		pr_alert("tvisor: failed to alloc vmxon_region\n");
		kfree(vm);
		return NULL;
	}
	pr_debug("tvisor: alloc vmxon region\n");

	vmcs_t *vmcs_region = alloc_vmcs_region();
	if (vmcs_region == NULL) {
		kfree(vm);
		free_vmxon_region(vmxon_region);
		return NULL;
	}

	pr_debug("tvisor: alloc vmcs region\n");

	const u64 size_mib = 0x100; // genkai

	ept_pointer_t *ept_pointer = create_ept_by_memsize(size_mib);
	if (ept_pointer == NULL) {
		kfree(vm);
		free_vmxon_region(vmxon_region);
		free_vmcs_region(vmcs_region);
		return NULL;
	}

	pr_debug("tvisor: alloc EPT[%llxMiB]\n", size_mib);
	const int order = 3; // 2 ^ 3 = 8 pages
	struct page *vmm_stack_pages = alloc_pages(GFP_KERNEL, order);
	if (vmm_stack_pages == NULL) {
		kfree(vm);
		free_vmxon_region(vmxon_region);
		free_vmcs_region(vmcs_region);
		free_ept(ept_pointer);
		return NULL;
	}
	u64 *vmm_stack = (u64 *)page_address(vmm_stack_pages);
	memset(vmm_stack, 0, 0x1000 * 8);

	pr_debug("tvisor: alloc vmm stack\n");

	struct page *msr_bitmap_page = alloc_page(GFP_KERNEL);
	if (msr_bitmap_page == NULL) {
		kfree(vm);
		free_vmxon_region(vmxon_region);
		free_vmcs_region(vmcs_region);
		free_ept(ept_pointer);
		__free_page(virt_to_page(vmm_stack));
		return NULL;
	}

	pr_debug("tvisor: alloc msr bitmap\n");

	vm->vmxon_region = vmxon_region;
	vm->vmcs_region = vmcs_region;
	vm->ept_pointer = ept_pointer;
	vm->vmm_stack = vmm_stack;
	vm->msr_bitmap_virt = (u64 *)page_address(msr_bitmap_page);
	vm->msr_bitmap_phys = __pa(vm->msr_bitmap_virt);

	return vm;
}

void destroy_vm(vm_state_t *vm)
{
	__free_page(virt_to_page(vm->msr_bitmap_virt));
	__free_page(virt_to_page(vm->vmm_stack));
	free_ept(vm->ept_pointer);
	free_vmcs_region(vm->vmcs_region);
	free_vmxon_region(vm->vmxon_region);
	kfree(vm);
}