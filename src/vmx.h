#pragma once

#include <linux/types.h>

typedef struct _vm_state {
	uint64_t vmxon_region;
	uint64_t vmcs_region;
} vm_state_t;

typedef struct _vmcs {
	uint32_t rev_id;
	uint32_t abort;
	uint32_t data[1];
} vmcs_t;

int init_vmx(void);
int exit_vmx(void);
int run_vmx(void);
