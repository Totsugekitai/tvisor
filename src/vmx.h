#pragma once

#include <linux/types.h>

typedef struct _vmcs {
	u32 rev_id;
	u32 abort;
	u32 data[1];
} vmcs_t;

int enable_vmx_on_each_cpu(void);
int disable_vmx_on_each_cpu(void);
int alloc_vmxon_region_all_cpu(void);
void free_vmxon_region_all_cpu(void);
