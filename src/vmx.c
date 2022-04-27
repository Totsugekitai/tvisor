#include <linux/printk.h> /* Needed for pr_alert */
#include <linux/mm.h> /* Needed for struct page, alloc_pages_node, page_address, etc... */
#include <linux/percpu-defs.h> /* Needed for DEFINE_PER_CPU macro */
#include <linux/slab.h> /* Needed for kmalloc */

#include <asm/msr.h>
#include <asm/processor.h>

#include "vmx.h"
#include "cpu.h"

static DEFINE_PER_CPU(vmcs_t *, vmxon_region);

static u32 is_vmx_supported(void)
{
	cpuid_t cpuid = get_cpuid(1);
	return (u32)(cpuid.ecx & 0b100000);
}

static u32 is_vmxon_supported(void)
{
	u64 ia32_feature_control = 0;
	rdmsrl(MSR_IA32_FEAT_CTL, ia32_feature_control);
	u64 lock = ia32_feature_control & 0b1;
	u64 vmxon_in_smx = ia32_feature_control & 0b10;
	u64 vmxon_outside_smx = ia32_feature_control & 0b100;

	return (u32)(lock | vmxon_in_smx | vmxon_outside_smx);
}

static vmcs_t *alloc_vmxon_region(int cpu)
{
	int node = cpu_to_node(cpu);

	u32 vmx_msr_low, vmx_msr_high;
	rdmsr(MSR_IA32_VMX_BASIC, vmx_msr_low, vmx_msr_high);

	struct page *page = alloc_pages_node(node, GFP_KERNEL, 0);
	if (page == NULL) {
		return NULL;
	}

	vmcs_t *vmxon_vmcs = page_address(page);

	size_t vmcs_size = vmx_msr_high & 0x1ffff;
	memset(vmxon_vmcs, 0, vmcs_size);

	vmxon_vmcs->rev_id = vmx_msr_low & 0x7fffffff;

	return vmxon_vmcs;
}

static void free_vmxon_region(int cpu)
{
	vmcs_t *vmxon_vmcs = per_cpu(vmxon_region, cpu);
	if (vmxon_vmcs == NULL) {
		return;
	}
	__free_page(virt_to_page(vmxon_vmcs));
}

static int vmxon(u64 phys_vmxon_region)
{
	u8 err;
	asm volatile("vmxon %1; setna %0"
		     : "=q"(err)
		     : "m"(phys_vmxon_region)
		     : "memory", "cc");

	return err;
}

static int vmxoff(void)
{
	u8 err;
	asm volatile("vmxoff; setna %0" : "=q"(err));

	return err;
}

static u64 vmptrst(void)
{
	u64 vmcspa = 0;
	asm volatile("vmptrst %0" : : "m"(&vmcspa) : "memory", "cc");

	pr_info("tvisor: VMPTRST %llx\n", vmcspa);

	return vmcspa;
}

static int vmclear(u64 vmcs_region)
{
	u8 err;
	asm volatile("vmclear %1; setna %0"
		     : "=q"(err)
		     : "m"(vmcs_region)
		     : "memory", "cc");

	return (int)err;
}

static int vmptrld(u64 vmcs_region)
{
	u8 err;
	asm volatile("vmptrld %1; setna %0" : "=q"(err) : "m"(vmcs_region));

	return (int)err;
}

static int enable_vmx(int cpu)
{
	vmcs_t *vmxon_vmcs = per_cpu(vmxon_region, cpu);
	if (vmxon_vmcs == NULL) {
		pr_alert("tvisor: vmxon region is not allocated[cpu %d]\n",
			 cpu);
		return -ENOMEM;
	}

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

	u64 phys_vmxon_vmcs = __pa(vmxon_vmcs);

	int err = vmxon(phys_vmxon_vmcs);
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

static void enable_vmx_per_cpu(void *data)
{
	int cpu = raw_smp_processor_id();
	int *err = (int *)data;
	err[cpu] = enable_vmx(cpu);
	if (err[cpu]) {
		pr_alert("tvisor: failed to enable VMX at cpu %d\n", cpu);
	}
}

static void disable_vmx_per_cpu(void *data)
{
	int cpu = raw_smp_processor_id();
	int *err = (int *)data;
	err[cpu] = disable_vmx(cpu);
	if (err[cpu]) {
		pr_alert("tvisor: failed to disable VMX at cpu %d\n", cpu);
	}
}

int enable_vmx_on_each_cpu(void)
{
	u32 b = is_vmx_supported() | is_vmxon_supported();
	if (!b) {
		return b;
	}

	size_t ncpu = num_online_cpus(), i;
	int *errs = (int *)kmalloc(sizeof(int) * ncpu, GFP_KERNEL);
	if (errs == NULL) {
		return -ENOMEM;
	}
	for (i = 0; i < ncpu; i++) {
		errs[i] = 0;
	}

	on_each_cpu(enable_vmx_per_cpu, errs, 1);

	for (i = 0; i < ncpu; i++) {
		if (errs[i]) {
			kfree(errs);
			return errs[i];
		}
	}

	kfree(errs);
	return 0;
}

int disable_vmx_on_each_cpu(void)
{
	size_t ncpu = num_online_cpus(), i;
	int *errs = (int *)kmalloc(sizeof(int) * ncpu, GFP_KERNEL);
	if (errs == NULL) {
		return -ENOMEM;
	}
	for (i = 0; i < ncpu; i++) {
		errs[i] = 0;
	}

	on_each_cpu(disable_vmx_per_cpu, errs, 1);

	for (i = 0; i < ncpu; i++) {
		if (errs[i]) {
			kfree(errs);
			return errs[i];
		}
	}

	kfree(errs);
	return 0;
}

int alloc_vmxon_region_all_cpu(void)
{
	int cpu;

	for_each_possible_cpu (cpu) {
		vmcs_t *vmxon_vmcs = alloc_vmxon_region(cpu);
		if (vmxon_vmcs == NULL) {
			return -ENOMEM;
		}

		per_cpu(vmxon_region, cpu) = vmxon_vmcs;
	}

	return 0;
}

void free_vmxon_region_all_cpu(void)
{
	int cpu;

	for_each_possible_cpu (cpu) {
		free_vmxon_region(cpu);
		per_cpu(vmxon_region, cpu) = NULL;
	}
}
