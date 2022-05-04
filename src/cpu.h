#pragma once

#include <linux/types.h>

typedef struct _cpuid {
	u32 eax;
	u32 ebx;
	u32 ecx;
	u32 edx;
} cpuid_t;

typedef union _segment_attributes {
	u16 all;
	struct {
		u16 type : 4;
		u16 s : 1;
		u16 dpl : 2;
		u16 p : 1;
		u16 avl : 1;
		u16 l : 1;
		u16 db : 1;
		u16 g : 1;
		u16 gap : 4;
	} fields;
} segment_attributes_t;

typedef struct _segment_selector {
	u16 sel;
	segment_attributes_t attrs;
	u32 limit;
	u64 base;
} segment_selector_t;

typedef struct _segment_descriptor {
	u16 limit0;
	u16 base0;
	u8 base1;
	u8 attr0;
	u8 limit1attr1;
	u8 base2;
} segment_descriptor_t;

enum SEGREGS { ES = 0, CS, SS, DS, FS, GS, LDTR, TR };

cpuid_t get_cpuid(u32 level);
u16 read_es(void);
u16 read_cs(void);
u16 read_ss(void);
u16 read_ds(void);
u16 read_fs(void);
u16 read_gs(void);
u64 read_cr3(void);
u64 read_cr4(void);
void write_cr4(u64 cr4);
u8 read_cpl(void);
int is_vmxe(void);
void set_vmxe(void);
u64 read_gdt_base(void);
u64 read_idt_base(void);
u16 read_gdt_limit(void);
u16 read_idt_limit(void);
u64 read_ldt(void);
u64 read_tr(void);
u64 read_rflags(void);
