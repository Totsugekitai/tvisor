#pragma once

#include <linux/types.h>

#include "ept.h"
#include "vmx.h"

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

typedef union _cr3 {
	u64 all;
	struct {
		u64 ignored1 : 3;
		u64 pwt : 1;
		u64 pcd : 1;
		u64 ingored2 : 7;
		u64 pml4_address : 36;
		u64 reserved : 16;
	} fields;
} cr3_t;

typedef union _pml4e {
	u64 all;
	struct {
		u64 present : 1;
		u64 read_write : 1;
		u64 user_supervisor : 1;
		u64 pwt : 1;
		u64 pcd : 1;
		u64 accessed : 1;
		u64 ignored1 : 1;
		u64 reserved1 : 1;
		u64 ignored2 : 4;
		u64 pdpt_address : 36;
		u64 reserved : 4;
		u64 ignored3 : 11;
		u64 xd : 1;
	} fields;
} pml4e_t;

typedef union _pdpte {
	u64 all;
	struct {
		u64 present : 1;
		u64 read_write : 1;
		u64 user_supervisor : 1;
		u64 pwt : 1;
		u64 pcd : 1;
		u64 accessed : 1;
		u64 ignored1 : 1;
		u64 page_size : 1;
		u64 ignored2 : 4;
		u64 pd_address : 36;
		u64 reserved : 4;
		u64 ignored3 : 11;
		u64 xd : 1;
	} fields;
} pdpte_t;

typedef union _pde {
	u64 all;
	struct {
		u64 present : 1;
		u64 read_write : 1;
		u64 user_supervisor : 1;
		u64 pwt : 1;
		u64 pcd : 1;
		u64 accessed : 1;
		u64 ignored1 : 1;
		u64 page_size : 1;
		u64 ignored2 : 4;
		u64 pt_address : 36;
		u64 reserved : 4;
		u64 ignored3 : 11;
		u64 xd : 1;
	} fields;
} pde_t;

typedef union _pte {
	u64 all;
	struct {
		u64 present : 1;
		u64 read_write : 1;
		u64 user_supervisor : 1;
		u64 pwt : 1;
		u64 pcd : 1;
		u64 accessed : 1;
		u64 dirty : 1;
		u64 pat : 1;
		u64 global : 1;
		u64 ignored2 : 3;
		u64 page_address : 36;
		u64 reserved : 4;
		u64 ignored3 : 7;
		u64 protection_key : 4;
		u64 xd : 1;
	} fields;
} __pte_t;

void launch_vm(int cpu, vm_state_t *vm);
vm_state_t *create_vm(void);
void destroy_vm(vm_state_t *vm);
cr3_t setup_guest_page_table(ept_pointer_t *eptp);
