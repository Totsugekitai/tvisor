#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>

#include "cpu.h"
// #include "msr.h"

int get_cpuid(uint32_t level, cpuid_t *cpuid)
{
	if (cpuid == NULL) {
		return -EINVAL;
	}

	asm_get_cpuid(level, &cpuid->eax, &cpuid->ebx, &cpuid->ecx,
		      &cpuid->edx);

	return 0;
}

uint64_t read_msr(uint32_t msr)
{
	uint32_t low, high;

	asm_read_msr(msr, &low, &high);
	return ((uint64_t)high << 32) | low;
}

void write_msr(uint32_t msr, uint64_t all)
{
	uint32_t low, high;

	low = (uint32_t)all;
	high = (uint32_t)(all >> 32);
	asm_write_msr(msr, low, high);
}

void enable_vmx(void *data)
{
	asm_enable_vmx();
}

void disable_vmx(void *data)
{
	asm_disable_vmx();
}

int is_vmx_supported(void)
{
	cpuid_t cpuid = { 0 };
	ia32_feature_control_msr_t control = { 0 };

	get_cpuid(1, &cpuid);

	if ((cpuid.ecx & (1 << 5)) == 0) {
		return 0;
	}

	control.all = read_msr(0x03a); /* MSR_IA32_FEATURE_CONTROL */

	if (control.fields.lock == 0) {
		control.fields.lock = 1;
		control.fields.enable_vmxon = 1;
		write_msr(0x03a, control.all); /* MSR_IA32_FEATURE_CONTROL */
	} else if (control.fields.enable_vmxon == 0) {
		pr_alert("VMX locked off in BIOS\n");
		return 0;
	}

	return 1;
}

int vmx_on(uint64_t phys)
{
	if (phys % 0x1000 != 0) {
		return -EINVAL;
	}
	asm_vmxon(phys);
	return 0;
}
