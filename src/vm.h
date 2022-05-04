#pragma once

#include <linux/types.h>

#include "vmx.h"
#include "ept.h"

#define VMM_STACK_SIZE 0x1000

typedef struct _vm_state {
	vmxon_region_t *vmxon_region;
	vmcs_t *vmcs_region;
	ept_pointer_t *ept_pointer;
	u64 *vmm_stack;
	u64 *msr_bitmap_virt;
	u64 msr_bitmap_phys;
	u64 rsp;
	u64 rbp;
} vm_state_t;

typedef struct _guest_regs {
	u64 rax;
	u64 rcx;
	u64 rdx;
	u64 rbx;
	u64 rsp;
	u64 rbp;
	u64 rsi;
	u64 rdi;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
} guest_regs_t;

void launch_vm(int cpu, vm_state_t *vm);
vm_state_t *create_vm(void);
void destroy_vm(vm_state_t *vm);
