#ifndef __KERNEL_MEMORY_H_
#define __KERNEL_MEMORY_H_
#include "stdint.h"
#include "bitmap.h"

/* 虚拟内存池 */
struct virtual_addr {
	struct bitmap vaddr_bitmap;	/*虚拟地址用的位图*/
	uint32_t vaddr_start;		/*虚拟地址的起始位置*/
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);
#endif /* __KERNEL_MEMORY_H_ */