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
} vm_state_t;
