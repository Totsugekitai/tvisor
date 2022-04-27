#pragma once

#include <linux/types.h>

typedef union _ept_pointer {
	u64 all;
	struct {
		u64 memory_type : 3;
		u64 page_walk_length : 3;
		u64 dirty_and_access_enabled : 1;
		u64 supervisor_shadow_stack : 1;
		u64 reserved1 : 4;
		u64 ept_pml4_table_address : 36;
		u64 reserved2 : 16;
	} fields;
} ept_pointer_t;

typedef union _ept_pml4e {
	u64 all;
	struct {
		u64 read : 1;
		u64 write : 1;
		u64 execute : 1;
		u64 reserved1 : 5;
		u64 accessed : 1;
		u64 ignored1 : 1;
		u64 execute_for_user_mode : 1;
		u64 ignored2 : 1;
		u64 ept_pdpt_address : 36;
		u64 reserved2 : 4;
		u64 ignored3 : 12;
	} fields;
} ept_pml4e_t;

typedef union _ept_pdpte {
	u64 all;
	struct {
		u64 read : 1;
		u64 write : 1;
		u64 execute : 1;
		u64 reserved1 : 5;
		u64 accessed : 1;
		u64 ignored1 : 1;
		u64 execute_for_user_mode : 1;
		u64 ignored2 : 1;
		u64 ept_pd_address : 36;
		u64 reserved2 : 4;
		u64 ignored3 : 12;
	} fields;
} ept_pdpte_t;

typedef union _ept_pde {
	u64 all;
	struct {
		u64 read : 1;
		u64 write : 1;
		u64 execute : 1;
		u64 reserved1 : 5;
		u64 accessed : 1;
		u64 ignored1 : 1;
		u64 execute_for_user_mode : 1;
		u64 ignored2 : 1;
		u64 ept_pt_address : 36;
		u64 reserved2 : 4;
		u64 ignored3 : 12;
	} fields;
} ept_pde_t;

typedef union _ept_pte {
	u64 all;
	struct {
		u64 read : 1;
		u64 write : 1;
		u64 execute : 1;
		u64 memory_type : 3;
		u64 ignore_pat : 1;
		u64 ignored1 : 1;
		u64 accessed : 1;
		u64 dirty : 1;
		u64 execute_for_user_mode : 1;
		u64 ignored2 : 1;
		u64 page_address : 36;
		u64 reserved : 4;
		u64 ignored3 : 8;
		u64 sss : 1;
		u64 sub_page_write_permission : 1;
		u64 ignored4 : 1;
		u64 suppress_ve : 1;
	} fields;
} ept_pte_t;