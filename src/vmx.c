#include <linux/printk.h> /* Needed for pr_alert */
#include <linux/mm.h> /* Needed for struct page, alloc_pages_node, page_address, etc... */
#include <linux/percpu-defs.h> /* Needed for DEFINE_PER_CPU macro */

#include <asm/msr.h>
#include <asm/processor.h>

#include "vmx.h"
#include "cpu.h"

static DEFINE_PER_CPU(vmcs_t *, vmcs_region);

static vmcs_t *alloc_vmcs_region(int cpu)
{
	int node = cpu_to_node(cpu);

	u32 vmx_msr_low, vmx_msr_high;
	rdmsr(MSR_IA32_VMX_BASIC, vmx_msr_low, vmx_msr_high);

	struct page *page = alloc_pages_node(node, GFP_KERNEL, 0);
	if (page == NULL) {
		return NULL;
	}

	vmcs_t *vmcs = page_address(page);

	size_t vmcs_size = vmx_msr_high & 0x1ffff;
	memset(vmcs, 0, vmcs_size);

	return vmcs;
}

static void free_vmcs_region(int cpu)
{
	vmcs_t *vmcs = per_cpu(vmcs_region, cpu);
	if (vmcs == NULL) {
		return;
	}
	__free_page(virt_to_page(vmcs));
}

static int vmxon(u64 phys)
{
	u8 err;
	asm volatile("vmxon %1; setna %0"
		     : "=q"(err)
		     : "m"(phys)
		     : "memory", "cc");

	return err;
}

static int vmxoff(void)
{
	u8 err;
	asm volatile("vmxoff; setna %0" : "=q"(err));
	if (err) {
		return err;
	}

	return 0;
}

static int enable_vmx(int cpu)
{
	u64 msr_vmx_basic = 0;
	rdmsrl(MSR_IA32_VMX_BASIC, msr_vmx_basic);

	vmcs_t *vmcs = per_cpu(vmcs_region, cpu);
	if (vmcs == NULL) {
		pr_alert("tvisor: vmcs is not allocated[cpu %d]\n", cpu);
		return -ENOMEM;
	}

	vmcs->rev_id = (u32)msr_vmx_basic;

	u64 cr0 = read_cr0();
	u64 msr_val = 0;
	rdmsrl(MSR_IA32_VMX_CR0_FIXED1, msr_val);
	cr0 &= msr_val;
	rdmsrl(MSR_IA32_VMX_CR0_FIXED0, msr_val);
	cr0 |= msr_val;
	write_cr0(cr0);

	u64 cr4 = read_cr4();
	rdmsrl(MSR_IA32_VMX_CR4_FIXED1, msr_val);
	cr4 &= msr_val;
	rdmsrl(MSR_IA32_VMX_CR4_FIXED0, msr_val);
	cr4 |= msr_val;
	write_cr4(cr4);

	u64 phys_vmcs = __pa(vmcs);

	int err = vmxon(phys_vmcs);
	if (err) {
		pr_alert("tvisor: failed to vmxon[%d] at cpu %d\n", err, cpu);
		return err;
	} else {
		pr_info("tvisor: vmxon success[cpu %d]\n", cpu);
		return 0;
	}
}

static int disable_vmx(int cpu)
{
	int err = vmxoff();
	if (err) {
		pr_alert("tvisor: vmxoff failed [%d] at cpu %d\n", err, cpu);
	} else {
		pr_info("tvisor: vmxoff success[cpu %d]\n", cpu);
	}
	return err;
}

static void enable_vmx_per_cpu(void *_dummy)
{
	int cpu = raw_smp_processor_id();
	if (enable_vmx(cpu)) {
		pr_alert("tvisor: failed to enable VMX at cpu %d\n", cpu);
	}
}

static void disable_vmx_per_cpu(void *_dummy)
{
	int cpu = raw_smp_processor_id();
	if (disable_vmx(cpu)) {
		pr_alert("tvisor: failed to disable VMX at cpu %d\n", cpu);
	}
}

void enable_vmx_all_cpu(void)
{
	on_each_cpu(enable_vmx_per_cpu, NULL, 1);
}

void disable_vmx_all_cpu(void)
{
	on_each_cpu(disable_vmx_per_cpu, NULL, 1);
}

int alloc_vmcs_all_cpu(void)
{
	int cpu;

	for_each_possible_cpu (cpu) {
		vmcs_t *vmcs = alloc_vmcs_region(cpu);
		if (vmcs == NULL) {
			return -ENOMEM;
		}

		per_cpu(vmcs_region, cpu) = vmcs;
	}

	return 0;
}

void free_vmcs_all_cpu(void)
{
	int cpu;

	for_each_possible_cpu (cpu) {
		free_vmcs_region(cpu);
		per_cpu(vmcs_region, cpu) = NULL;
	}
}
