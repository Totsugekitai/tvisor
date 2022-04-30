#pragma once

#include <linux/types.h>

#ifdef DEBUG
#define dprintf(fmt, ...) pr_info(fmt, __VA_ARGS__);
#else
#define dprintf(fmt, ...)
#endif

u64 power(u64 base, size_t exp);