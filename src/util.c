#include "util.h"

u64 power(u64 base, size_t exp)
{
	u64 a = base;
	size_t i;
	for (i = 0; i < exp; i++) {
		a *= a;
	}
	return a;
}
