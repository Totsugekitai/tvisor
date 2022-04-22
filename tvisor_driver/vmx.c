#include <linux/printk.h> /* Needed for pr_alert */
#include <linux/mm.h> /* Needed for struct page, __alloc_pages_node, page_address, etc... */
#include <linux/percpu-defs.h> /* Needed for DEFINE_PER_CPU macro */

// #include <asm/io.h> /* Needed for virt_to_phys */
#include <asm/msr.h>
#include <asm/processor.h>

#include "vmx.h"
#include "cpu.h"

static DEFINE_PER_CPU(vmcs_t *, vmxon_region);
static vmcs_t *vmcs;

static vmcs_t *alloc_vmcs_region(int cpu)
{
	u32 vmx_msr_low, vmx_msr_high;

	int node = cpu_to_node(cpu);

	rdmsr(MSR_IA32_VMX_BASIC, vmx_msr_low, vmx_msr_high);
	size_t vmcs_size = vmx_msr_high & 0x1ffff;

	struct page *page = __alloc_pages_node(node, GFP_KERNEL, 0);
	if (page == NULL) {
		return NULL;
	}

	vmcs_t *vmcs = page_address(page);
	memset(vmcs, 0, vmcs_size);

	return vmcs;
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

static void enable_vmx(void *_dummy)
{
	int cpu = raw_smp_processor_id();

	u64 msr_vmx_basic = 0;
	rdmsrl(MSR_IA32_VMX_BASIC, msr_vmx_basic);

	vmcs_t *vmxon_vmcs = per_cpu(vmxon_region, cpu);

	vmxon_vmcs->rev_id = (u32)msr_vmx_basic;

	u64 cr0 = read_cr0();
	u64 msr = 0;
	rdmsrl(MSR_IA32_VMX_CR0_FIXED1, msr);
	cr0 &= msr;
	rdmsrl(MSR_IA32_VMX_CR0_FIXED0, msr);
	cr0 |= msr;
	write_cr0(cr0);

	u64 cr4 = read_cr4();
	rdmsrl(MSR_IA32_VMX_CR4_FIXED1, msr);
	cr4 &= msr;
	rdmsrl(MSR_IA32_VMX_CR4_FIXED0, msr);
	cr4 |= msr;
	write_cr4(cr4);

	u64 phys_vmxon_vmcs = __pa(vmxon_vmcs);

	int err = vmxon(phys_vmxon_vmcs);
	if (err) {
		pr_alert("tvisor: failed to vmxon[%d]\n", err);
	}
}

int run_vmx(void)
{
	on_each_cpu(enable_vmx, NULL, 1);
	return 0;
}

int init_vmx(void)
{
	int cpu;

	for_each_possible_cpu (cpu) {
		vmcs_t *vmcs = alloc_vmcs_region(cpu);
		if (vmcs == NULL) {
			return -ENOMEM;
		}

		per_cpu(vmxon_region, cpu) = vmcs;
	}

	return 0;
}

static void exit_vmx_inner(void *_dummy)
{
	int cpu = raw_smp_processor_id();

	int err = vmxoff();
	if (err) {
		pr_alert("tvisor: vmxoff failed[%d]\n", err);
		return;
	}

	__free_page(virt_to_page(per_cpu(vmxon_region, cpu)));

	vmcs = NULL;
	vmxon_region = NULL;

	pr_info("tvisor: vmxoff success[cpu %d]\n", cpu);
}

int exit_vmx(void)
{
	on_each_cpu(exit_vmx_inner, NULL, 1);

	return 0;
}