#include <linux/errno.h>
#include <linux/kernel.h>

#include "cpu.h"

int get_cpuid(u32 level, cpuid_t *cpuid)
{
	if (cpuid == NULL) {
		return -EINVAL;
	}

	asm volatile("cpuid"
		     : "=a"(cpuid->eax), "=b"(cpuid->ebx), "=c"(cpuid->ecx),
		       "=d"(cpuid->edx)
		     : "r"(level));

	return 0;
}

u16 read_es(void)
{
	u16 es;
	asm volatile("mov %%es, %0" : "=r"(es));
	return es;
}

u16 read_cs(void)
{
	u16 cs;
	asm volatile("mov %%cs, %0" : "=r"(cs));
	return cs;
}

u16 read_ss(void)
{
	u16 ss;
	asm volatile("mov %%ss, %0" : "=r"(ss));
	return ss;
}

u16 read_ds(void)
{
	u16 ds;
	asm volatile("mov %%ds, %0" : "=r"(ds));
	return ds;
}

u16 read_fs(void)
{
	u16 fs;
	asm volatile("mov %%fs, %0" : "=r"(fs));
	return fs;
}

u16 read_gs(void)
{
	u16 gs;
	asm volatile("mov %%gs, %0" : "=r"(gs));
	return gs;
}

u64 read_cr4(void)
{
	u64 cr4;
	asm volatile("movq %%cr4, %0" : "=r"(cr4));

	return cr4;
}

void write_cr4(u64 cr4)
{
	asm volatile("movq %0, %%cr4" : : "r"(cr4));
}

u8 read_cpl(void)
{
	u16 cs;
	asm volatile("mov %%cs, %0" : "=r"(cs));
	return (u8)cs & 3;
}

// int is_vmx_supported(void)
// {
// 	cpuid_t cpuid = { 0 };
// 	ia32_feature_control_msr_t control = { 0 };

// 	get_cpuid(1, &cpuid);

// 	if ((cpuid.ecx & (1 << 5)) == 0) {
// 		return 0;
// 	}

// 	control.all = read_msr(0x03a); /* MSR_IA32_FEATURE_CONTROL */

// 	if (control.fields.lock == 0) {
// 		control.fields.lock = 1;
// 		control.fields.enable_vmxon = 1;
// 		write_msr(0x03a, control.all); /* MSR_IA32_FEATURE_CONTROL */
// 	} else if (control.fields.enable_vmxon == 0) {
// 		pr_alert("VMX locked off in BIOS\n");
// 		return 0;
// 	}

// 	return 1;
// }
