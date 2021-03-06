#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "global.h"
#include "debug.h"
#include "string.h"

#define PAGE_SIZE 4096

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

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

/*
*	在pf表示的虚拟内存池中申请pg_cnt个虚拟页
*	成功则返回虚拟页的起始地址,否则返回NULL
*/
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
	int32_t vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0;
	if (pf == PF_KERNEL) {
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if (bit_idx_start == -1) {
			return NULL;
		}
		while(cnt < pg_cnt) {
			bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + (cnt++), 1);
		}
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PAGE_SIZE;
	} else {
		/* 用户内存池 */
	}
	return (void*)vaddr_start;
}

/* 	得到虚拟地址的vaddr所对应的PDE指针 
* 	分析写vaddr的结构       前10个bit是在页目录表下的下表
* 	中间10个bit是页框下页表所在的下表
* 	最后12个bit是物理地址所在页表所指内存的偏移地址
*/
uint32_t* pde_ptr(uint32_t vaddr) {
	/*
	*	1. 首先获得页框的首地址
	*	2. 获得所在下表*4
	*/
	uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
	return pde;
}

/*
*	得到虚拟地址的vaddr所对应的PTE指针
*/
uint32_t* pte_ptr(uint32_t vaddr) {
	/*
	*	1. 首先访问页表自己
	*	2. 再用页目录项的下表作为pet的索引访问页表
	*	2. 再用pte的偏移地址作为页内偏移
	*/
	uint32_t *pte = (uint32_t*)(0xffc00000 + 
					((vaddr & 0xffc00000) >> 10) +
					PTE_IDX(vaddr) * 4);
	return pte;
}

/* 	在m_pool指向的物理内存池中分配1个物理页
*	成功返回页框的物理地址, 失败返回NULL
*/
static void* palloc(struct pool* m_pool) {
	/*扫描或者设置位图保证原子操作 !!! ??? */
	int32_t bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if (bit_idx == -1) {
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, bit_idx,  1);
	uint32_t page_phyaddr = ((bit_idx) * PAGE_SIZE + m_pool->phy_addr_start);
	return (void* )page_phyaddr;
}

/* 在页表中添加虚拟地址和物理地址的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);

	/* 	执行*pde会遇到pde是空的的情况。所以需要确保pde创建完成后才执行*pte 
	*	否则会引发page_fault.
	*/
	/* 判断页是否存在的方式 PG_P标志 */
	if (*pde & 0x00000001) {
		// 此处假定pte一定不存在
		ASSERT(!(*pte & 0x00000001));
		if (!(*pte & 0x00000001)) { /* 不存在pte */
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		} else {
			PANIC("pte repeat!");
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		}
	} else { /* 页目录框不存在 */
		/*页表中用到的页框要到内存中分配*/
		uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);

		*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		/* 初始化分配的到的页框的内容 */
		memset((void*)(0xfffff000 & (uint32_t)pte), 0, PAGE_SIZE);
		ASSERT(!(*pte & 0x00000001));
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}
}


/* 分配pg_cnt个页空间, 成功返回起始虚拟地址, 失败返回NULL */
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
	ASSERT(pg_cnt > 0 && pg_cnt < 3840); /* 3840 = 15M / 4K */
	void *start_vaddr = vaddr_get(pf, pg_cnt);
	if (start_vaddr == NULL) {
		return NULL;
	}
	uint32_t vaddr = (uint32_t)start_vaddr, cnt = pg_cnt;
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

	while(cnt-- > 0) {
		void* page_phyaddr = palloc(mem_pool);			/* 申请物理地址 */
		if (page_phyaddr == NULL) {
			// 物理也回滚操作 ???
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr);		/* 页表中映射 */
		vaddr += PAGE_SIZE;
	}
	return start_vaddr;
}


/* 从内核物理内存池中申请pg_cnt页内存
*	成功返回虚拟地址,失败返回NULL
*/
void* get_kernel_pages(uint32_t pg_cnt) {
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if (vaddr != NULL) {
		memset(vaddr, 0, pg_cnt * PAGE_SIZE);
	}
	return vaddr;
}

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





















