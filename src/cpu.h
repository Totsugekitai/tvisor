#pragma once

#include <linux/types.h>

typedef struct _cpuid {
	u32 eax;
	u32 ebx;
	u32 ecx;
	u32 edx;
} cpuid_t;

// typedef union _ia32_feature_control_msr {
// 	uint64_t all;
// 	struct {
// 		uint64_t lock : 1;
// 		uint64_t enable_smx : 1;
// 		uint64_t enable_vmxon : 1;
// 		uint64_t reserved2 : 5;
// 		uint64_t enable_local_senter : 7;
// 		uint64_t enable_global_senter : 1;
// 		uint64_t reserved3a : 16;
// 		uint64_t reserved3b : 32;
// 	} fields;
// } ia32_feature_control_msr_t;

// typedef union _ia32_vmx_basic_msr {
// 	uint64_t all;
// 	struct {
// 		uint32_t revision_id : 31;
// 		uint32_t reserved1 : 1;
// 		uint32_t region_size : 12;
// 		uint32_t region_clear : 1;
// 		uint32_t reserved2 : 3;
// 		uint32_t supported_ia64 : 1;
// 		uint32_t supported_dual_monitor : 1;
// 		uint32_t memory_type : 4;
// 		uint32_t vmexit_report : 1;
// 		uint32_t vmx_capability_hint : 1;
// 		uint32_t reserved3 : 8;
// 	} fields;
// } ia32_vmx_basic_msr_t;

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
