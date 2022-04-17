#ifndef CPU_H
#define CPU_H

#include <linux/types.h>
#include <linux/module.h>

/* in cpu_asm.S */
extern void asm_get_cpuid(uint32_t level, uint32_t *eax, uint32_t *ebx,
			  uint32_t *ecx, uint32_t *edx);
extern void asm_read_msr(uint32_t msr, uint32_t *low, uint32_t *high);
extern void asm_write_msr(uint32_t msr, uint32_t low, uint32_t high);

extern uint64_t is_vmx_enabled(void);
extern void asm_enable_vmx(void);
extern void asm_disable_vmx(void);

/* in cpu.c */
typedef struct _cpuid {
	int eax;
	int ebx;
	int ecx;
	int edx;
} cpuid_t;

typedef union _ia32_feature_control_msr {
	uint64_t all;
	struct {
		uint64_t lock : 1;
		uint64_t enable_smx : 1;
		uint64_t enable_vmxon : 1;
		uint64_t reserved2 : 5;
		uint64_t enable_local_senter : 7;
		uint64_t enable_global_senter : 1;
		uint64_t reserved3a : 16;
		uint64_t reserved3b : 32;
	} fields;
} ia32_feature_control_msr_t;

int is_vmx_supported(void);
void enable_vmx(void *data);
void disable_vmx(void *data);

#endif