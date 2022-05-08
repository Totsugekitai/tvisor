#include <linux/cpumask.h> /* Needed for cpumask_* */
#include <linux/mm.h> /* Needed for page_address */
#include <linux/printk.h> /* Needed for printk */
#include <linux/slab.h> /* Needed for kmalloc */
#include <linux/smp.h> /* Needed for on_each_cpu_mask */

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
	pr_debug("tvisor: rsp=%llx, rbp=%llx\n", vm->rsp, vm->rbp);

	vmlaunch();

	u64 err = vmread(VM_INSTRUCTION_ERROR);
	pr_info("tvisor: vmlaunch is failed\n");
	pr_debug("tvisor: vm instruction error[%lld]\n", err);
	vmxoff();
	TVISOR_STATE.is_vmx_enabled = 0;
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
	vm = NULL;
}

cr3_t setup_sample_guest_page_table(ept_pointer_t *eptp)
{
	// const u64 pml4_gphys = 0x1000;
	// const u64 pdpt_gphys = 0x2000;
	// const u64 pd_gphys = 0x3000;
	// const u64 pt_gphys = 0x4000;

	u64 pa_ept_pml4 = eptp->fields.ept_pml4_table_address << 12;
	ept_pml4e_t *va_ept_pml4 = __va(pa_ept_pml4);
	u64 pa_ept_pdpt = va_ept_pml4[0].fields.ept_pdpt_address << 12;
	ept_pdpte_t *va_ept_pdpt = __va(pa_ept_pdpt);
	u64 pa_ept_pd = va_ept_pdpt[0].fields.ept_pd_address << 12;
	ept_pde_t *va_ept_pd = __va(pa_ept_pd);
	u64 pa_ept_pt = va_ept_pd[0].fields.ept_pt_address << 12;
	ept_pte_t *va_ept_pt = __va(pa_ept_pt);

	u64 pa_pml4 = va_ept_pt[1].fields.page_address << 12;
	u64 pa_pdpt = va_ept_pt[2].fields.page_address << 12;
	u64 pa_pd = va_ept_pt[3].fields.page_address << 12;
	u64 pa_pt = va_ept_pt[4].fields.page_address << 12;
	pml4e_t *va_pml4 = __va(pa_pml4);
	pdpte_t *va_pdpt = __va(pa_pdpt);
	pde_t *va_pd = __va(pa_pd);
	__pte_t *va_pt = __va(pa_pt);
	memset(va_pml4, 0, 0x1000);
	memset(va_pdpt, 0, 0x1000);
	memset(va_pd, 0, 0x1000);
	memset(va_pt, 0, 0x1000);

	cr3_t cr3;
	cr3.all = 0;
	cr3.fields.pml4_address = 1;

	va_pml4[0].fields.present = 1;
	va_pml4[0].fields.pdpt_address = 2;

	va_pdpt[0].fields.present = 1;
	va_pdpt[0].fields.pd_address = 3;

	va_pd[0].fields.present = 1;
	va_pd[0].fields.pt_address = 4;

	size_t i;
	for (i = 0; i < 512; i++) {
		va_pt[i].fields.page_address = 0;
		va_pt[i].fields.present = 1;
	}

	return cr3;
}