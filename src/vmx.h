#pragma once

#include <linux/types.h>

#include "ept.h"

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

int enable_vmx_on_each_cpu_mask(int cpu, vmcs_t *vmxon_region);
int disable_vmx_on_each_cpu_mask(int cpu);

int clear_vmcs_state(vmcs_t *vmcs);
int load_vmcs(vmcs_t *vmcs);
int setup_vmcs(vmcs_t *vmcs, ept_pointer_t *eptp);
inline void save_vmxoff_state(u64 *rsp, u64 *rbp);
int vmlaunch(void);
