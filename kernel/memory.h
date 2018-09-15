#ifndef __KERNEL_MEMORY_H_
#define __KERNEL_MEMORY_H_
#include "stdint.h"
#include "bitmap.h"

enum pool_flags {
	PF_KERNEL = 1,  /* 内核内存池 */
	PF_USER = 2		/* 用户内存池 */
};

#define	PG_P_1 	1  	/* 页表项和页目录项存在标志位 */
#define PG_P_0	0	/**/
#define PG_RW_R	0	/* RW可读	*/
#define PG_RW_W	2	/* RW可写 */
#define PG_US_S 0	/* U/S标志 系统级 */
#define PG_US_U	4	/* U/S标志 用户级 */

/* 虚拟内存池 */
struct virtual_addr {
	struct bitmap vaddr_bitmap;	/*虚拟地址用的位图*/
	uint32_t vaddr_start;		/*虚拟地址的起始位置*/
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);
#endif /* __KERNEL_MEMORY_H_ */