#pragma once

#include <linux/types.h>

typedef struct _vm_state {
	u64 vmxon_region;
	u64 vmcs_region;
	u64 ept_pointer;
	u64 vmm_stack;
	u64 msr_bitmap_virt;
	u64 msr_bitmap_phys;
} vm_state_t;
