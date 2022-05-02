#pragma once

#include <linux/types.h>

#include "vmx.h"
#include "ept.h"

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

void launch_vm(int cpu, vm_state_t *vm);
vm_state_t *create_vm(void);
void destroy_vm(vm_state_t *vm);
