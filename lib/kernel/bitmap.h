#ifndef __LIB_KERNEL_BITMAP_H_
#define __LIB_KERNEL_BITMAP_H_
#include "global.h"
#define BITMAP_MASK 1
struct bitmap {
	uint32_t btmp_bytes_len;
	uint8_t* bits;
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value); 
#endif
