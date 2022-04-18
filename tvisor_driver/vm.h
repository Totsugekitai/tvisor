#ifndef VM_H
#define VM_H

#include <linux/types.h>

typedef struct _vm_state {
	uint64_t vmxon_region;
	uint64_t vmcs_region;
} vm_state_t;

int allocate_vmxon_region(vm_state_t *vm_state);

#endif