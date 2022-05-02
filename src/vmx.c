#include <linux/printk.h> /* Needed for pr_alert */
#include <linux/mm.h> /* Needed for struct page, alloc_pages_node, page_address, etc... */
#include <linux/percpu-defs.h> /* Needed for DEFINE_PER_CPU macro */
#include <linux/slab.h> /* Needed for kmalloc */

#include <asm/msr.h>
#include <asm/processor.h>

#include "vmx.h"
#include "cpu.h"
#include "ept.h"

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

int vmlaunch(void)
{
	u8 err;
	asm volatile("vmlaunch; setna %0" : "=q"(err));
	return (int)err;
}

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

vmcs_t *alloc_vmcs_region(void)
{
	u32 vmx_msr_low, vmx_msr_high;
	rdmsr(MSR_IA32_VMX_BASIC, vmx_msr_low, vmx_msr_high);

	struct page *page = alloc_page(GFP_KERNEL);
	if (page == NULL) {
		return NULL;
	}

	vmcs_t *vmcs = page_address(page);

	size_t vmcs_size = vmx_msr_high & 0x1ffff;
	memset(vmcs, 0, vmcs_size);

	vmcs->rev_id = vmx_msr_low & 0x7fffffff;

	return vmcs;
}

vmxon_region_t *alloc_vmxon_region(void)
{
	return (vmxon_region_t *)alloc_vmcs_region();
}

void free_vmcs_region(vmcs_t *vmcs)
{
	if (vmcs == NULL) {
		return;
	}
	__free_page(virt_to_page(vmcs));
}

void free_vmxon_region(vmxon_region_t *vmxon_region)
{
	free_vmcs_region((vmcs_t *)vmxon_region);
}

static int enable_vmx(vmcs_t *vmxon_region)
{
	vmcs_t *vmxon_vmcs = vmxon_region;
	if (vmxon_vmcs == NULL) {
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

	return vmxon(phys_vmxon_vmcs);
}

static int disable_vmx(void)
{
	return vmxoff();
}

static void enable_vmx_per_cpu(void *data)
{
	vmcs_t *vmxon_region = (vmcs_t *)data;
	int err = enable_vmx(vmxon_region);
	if (err) {
		pr_alert("tvisor: failed to enable VMX[%d]\n", err);
	}
}

static void disable_vmx_per_cpu(void *_dummy)
{
	int err = disable_vmx();
	if (err) {
		pr_alert("tvisor: failed to disable VMX[%d]\n", err);
	}
}

int enable_vmx_on_each_cpu(vmcs_t *vmxon_region)
{
	u32 b = is_vmx_supported() | is_vmxon_supported();
	if (!b) {
		return b;
	}

	on_each_cpu(enable_vmx_per_cpu, vmxon_region, 1);

	return 0;
}

int disable_vmx_on_each_cpu(void)
{
	on_each_cpu(disable_vmx_per_cpu, NULL, 1);

	return 0;
}

int enable_vmx_on_each_cpu_mask(int cpu, vmcs_t *vmxon_region)
{
	u32 b = is_vmx_supported() | is_vmxon_supported();
	if (!b) {
		return b;
	}

	struct cpumask mask;
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	on_each_cpu_mask(&mask, enable_vmx_per_cpu, vmxon_region, 1);

	return 0;
}

int disable_vmx_on_each_cpu_mask(int cpu)
{
	struct cpumask mask;
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	on_each_cpu_mask(&mask, disable_vmx_per_cpu, NULL, 1);

	return 0;
}

int clear_vmcs_state(vmcs_t *vmcs)
{
	u64 vmcs_phys = __pa(vmcs);
	return vmclear(vmcs_phys);
}

int load_vmcs(vmcs_t *vmcs)
{
	u64 vmcs_phys = __pa(vmcs);
	return vmptrld(vmcs_phys);
}

int setup_vmcs(vmcs_t *vmcs, ept_pointer_t *eptp)
{
	// TODO: implementation
	return 0;
}

inline void save_vmxoff_state(u64 *rsp, u64 *rbp)
{
	asm volatile("mov %%rsp,%0; mov %%rbp, %1"
		     : "=m"(*rsp), "=m"(*rbp)
		     :
		     : "memory", "cc");
}