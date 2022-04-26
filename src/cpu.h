#pragma once

#include <linux/types.h>

typedef struct _cpuid {
	u32 eax;
	u32 ebx;
	u32 ecx;
	u32 edx;
} cpuid_t;

cpuid_t get_cpuid(u32 level);
u16 read_es(void);
u16 read_cs(void);
u16 read_ss(void);
u16 read_ds(void);
u16 read_fs(void);
u16 read_gs(void);
u64 read_cr4(void);
void write_cr4(u64 cr4);
u8 read_cpl(void);
int is_vmxe(void);
void set_vmxe(void);
