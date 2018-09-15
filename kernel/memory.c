#include "memory.h"
#include "stdint.h"
#include "print.h"

#define PAGE_SIZE 4096

/* 	位图地址
*	0xc009f000表示内核主线程的栈顶, 0xc009e000是内核主线程的pcb(程序控制块)
*	一个页框大小的位图可以表示128M的内存: 4K * 8 * 4K = 128M
*	位图地址安排在0xc009a000,这样就支持4个页框，最多表示512M的内存
*/
#define MEM_BITMAP_BASE	0xc009a000

/* 	0xc0000000 是内核的起始地址, 从虚拟地址的3G开始	
*	跳过内核开始的1M 0x00100000
*/
#define K_HEAP_START 0xc0100000

/*内存池结构*/
struct pool {
	struct bitmap pool_bitmap;	/* 内存池使用的位图结构 */
	uint32_t phy_addr_start;	/* 内存池的物理内存地址起始 */
	uint32_t pool_size;			/* 内存池字节大小 */
};

struct pool kernel_pool, user_pool;		/* 生成内核内存池和用户内存池 */
struct virtual_addr kernel_vaddr;		/* 内核虚拟地址 */

/*初始化内存池*/
static void mem_pool_init(uint32_t all_mem) {
	put_str("  mem_pool_init start.\n");
	uint32_t page_table_size = PAGE_SIZE * 256; 
	/* 0~768指向同一个页框, 769~1022指向254个页框,       目录项占一个页框       */
	uint32_t used_mem = page_table_size + 0x100000; /* 跳过1M */
	uint32_t free_mem = all_mem - used_mem;
	uint16_t all_free_pages = free_mem / PAGE_SIZE;
	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages; 

	/* 简化位图处理， 不考虑余数， 会丢内存*/
	uint32_t kbm_length = kernel_free_pages / 8;	/* 内核位图字节长度 */
	uint32_t ubm_length = user_free_pages / 8;		/* 用户位图字节长度 */

	uint32_t kp_start = used_mem;
	uint32_t up_start = kp_start + kernel_free_pages * PAGE_SIZE;

	/*	内核内存池和用户内存池
	*	位图是全局变量长度不固定
	*	全局或静态变量需要在编译时确定其长度
	* 	所以需要根据总内存长度算出内存位图的长度
	*/
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;

	kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
	user_pool.pool_size = user_free_pages * PAGE_SIZE;

	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;


	/*
	*	内核使用最高地址是0xc009f000
	*/
	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE; /*  */
	user_pool.pool_bitmap.bits = (void*) (MEM_BITMAP_BASE + kbm_length);

	/* 输出内存池信息 */
	put_str("    kernel_pool_bitmap_start: ");
	put_int((uint32_t)(kernel_pool.pool_bitmap.bits));
	put_str(" kernel_pool_phy_addr_start: ");
	put_int(kernel_pool.phy_addr_start);
	put_char('\n');
	put_str("    user_pool_bitmap_start: ");
	put_int((uint32_t)(user_pool.pool_bitmap.bits));
	put_str(" user_pool_phy_addr_start: ");
	put_int(user_pool.phy_addr_start);
	put_char('\n');

	/* 将位图置0 */
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);

	/* 初始化内核虚拟地址的位图， 按实际物理内存大小生成数组 */
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;

	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);
	put_str("  mem_pool_init done\n");
}

/* 内存管理部分初始化入口 */
void mem_init(void) {
	put_str("mem_init start.\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb03));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done.\n");
}





















