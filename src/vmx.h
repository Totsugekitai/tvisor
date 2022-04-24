#pragma once

#include <linux/types.h>

typedef struct _vm_state {
	u64 vmxon_region;
	u64 vmcs_region;
} vm_state_t;

typedef struct _vmcs {
	u32 rev_id;
	u32 abort;
	u32 data[1];
} vmcs_t;

void enable_vmx_all_cpu(void);
void disable_vmx_all_cpu(void);
int alloc_vmcs_all_cpu(void);
void free_vmcs_all_cpu(void);
