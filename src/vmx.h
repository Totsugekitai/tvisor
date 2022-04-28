#pragma once

#include <linux/types.h>

typedef struct _vmcs {
	u32 rev_id;
	u32 abort;
	u32 data[1];
} vmcs_t;

typedef vmcs_t vmxon_region_t;

vmcs_t *alloc_vmcs_region(void);
vmxon_region_t *alloc_vmxon_region(void);
void free_vmcs_region(vmcs_t *vmcs);
void free_vmxon_region(vmxon_region_t *vmxon_region);

int enable_vmx_on_each_cpu(vmcs_t *vmxon_region);
int disable_vmx_on_each_cpu(void);
