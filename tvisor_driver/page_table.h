#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <linux/types.h>

uint64_t vaddr2paddr(uint64_t vaddr);

#endif