#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "debug.h"
#include "interrupt.h"
#include "list.h"
#include "print.h"

#define PG_SIZE 4096

struct task_struct* main_thread;		/* 主进程PCB 0xc009e000 */
struct list thread_ready_list;			/* 线程队列 */
struct list thread_all_list;		 	/* 所有任务队列 */
static struct list_elem* thread_tag; 	/* 用于保存线程队列中的节点 */

extern void switch_to(struct task_struct* cur, struct task_struct* next);

/* 获取当前PCB指针 */
struct task_struct* running_thread() {
	uint32_t esp;
	asm volatile("movl %%esp, %0":"=r"(esp));
	return (struct task_struct*)(esp & 0xfffff000);
}



/* 由kernel_thread去执行function */
static void kernel_thread(thread_func* function, void* func_arg) {
	intr_enable();
	function(func_arg);
}

/* 初始化线程栈 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {
	/*预留中断栈空间*/
	pthread->self_kstack -= sizeof(struct intr_stack);
	/*预留线程栈空间*/
	pthread->self_kstack -= sizeof(struct thread_stack);

	struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_arg = func_arg;
	kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

/* 初始化线程基本信息 */
void init_thread(struct task_struct* pthread, char* name, int prio) {
	memset(pthread, 0, sizeof(*pthread));
	strcpy(pthread->name, name);
	if (pthread == main_thread) {
		pthread->status = TASK_RUNNING;
	} else {
		pthread->status = TASK_READY;
	}
	pthread->priority = prio;
	/* self_kstack是线程自己在内核下使用的栈顶地址 */
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->ticks = prio;
	pthread->elapsed_ticks = 0;
	pthread->pgdir = NULL;
	pthread->stack_magic = 0x12345678;	/*自定义魔数*/
}

/*创建优先级为prio的线程,名为name,执行函数为function(func_arg)*/
struct task_struct* thread_start(char* name, int prio, thread_func* function, void* func_arg) {
	/*pcb 都位于内核空间*/
	struct task_struct* thread = get_kernel_pages(1);
	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);
	/* 确保不在队列中 */
	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
	/* 加入就绪队列 */
	list_append(&thread_ready_list, &thread->general_tag);
	/* 确保不再队列中 */
	ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag);

	return thread;
}

static void make_main_thread(void) {
	/* 	因为main已经在运行
	*	进入内核时的esp是0xc009c000
	*	0xc009e000是预留给main的PCB*/
	main_thread = running_thread();
	/* main已经在运行,所以不加在thread_ready_list中 */
	init_thread(main_thread, "main",  31);
	ASSERT(!elem_find(&thread_all_list,  &main_thread->all_list_tag));
	list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule() {
	/* 关中断 */
	ASSERT(intr_get_status() == INTR_OFF);
	struct task_struct* cur = running_thread();
	if (cur->status == TASK_RUNNING) {
		/* 时间片到了 */
		ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	} else {
		/* 如果需要发生其他条件才能在cpu上运行, 那么不加在队列上 */
	}
	ASSERT(!list_empty(&thread_ready_list));	/* 运行队列不为空 */
	thread_tag = NULL;
	thread_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
	next->status = TASK_RUNNING;
	switch_to(cur,next);

}

/* 初始化线程环境 */
void thread_init() {
	put_str("thread_init start\n");
	list_init(&thread_all_list);
	list_init(&thread_ready_list);
	make_main_thread();
	put_str("thread_init done\n");
}
