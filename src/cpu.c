#include "cpu.h"

cpuid_t get_cpuid(u32 level)
{
	cpuid_t cpuid = { 0, 0, 0, 0 };
	asm volatile("cpuid"
		     : "=a"(cpuid.eax), "=b"(cpuid.ebx), "=c"(cpuid.ecx),
		       "=d"(cpuid.edx)
		     : "r"(level));

	return cpuid;
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

u64 read_cr3(void)
{
	u64 cr3;
	asm volatile("movq %%cr3, %0" : "=r"(cr3));

	return cr3;
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

int is_vmxe(void)
{
	u64 cr4 = 0;
	asm volatile("mov %%cr4, %0" : "=r"(cr4));
	return (int)(cr4 & 0x2000);
}

void set_vmxe(void)
{
	asm volatile("mov %%cr4, %%rax; or $0x2000, %%rax; mov %%rax, %%cr4"
		     :
		     :
		     : "%rax");
}

u64 read_gdt_base(void)
{
	char gdtr[10];
	asm volatile("sgdt %0" : "=m"(gdtr) : : "memory", "cc");
	return *(u64 *)(&gdtr[2]);
}

u16 read_gdt_limit(void)
{
	char gdtr[10];
	asm volatile("sgdt %0" : "=m"(gdtr) : : "memory", "cc");
	return *(u16 *)(gdtr);
}

u64 read_idt_base(void)
{
	char idtr[10];
	asm volatile("sidt %0" : "=m"(idtr) : : "memory", "cc");
	return *(u64 *)(&idtr[2]);
}

u16 read_idt_limit(void)
{
	char idtr[10];
	asm volatile("sgdt %0" : "=m"(idtr) : : "memory", "cc");
	return *(u16 *)(idtr);
}

u64 read_ldt(void)
{
	u64 ldtr;
	asm volatile("sldt %0;" : "=r"(ldtr));
	return ldtr;
}

u64 read_tr(void)
{
	u64 tr;
	asm volatile("str %0;" : "=r"(tr));
	return tr;
}

u64 read_rflags(void)
{
	u64 rflags;
	asm volatile("pushfq; pop %%rax; mov %%rax, %0"
		     : "=r"(rflags)
		     :
		     : "rax");
	return rflags;
}