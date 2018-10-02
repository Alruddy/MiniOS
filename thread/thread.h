#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"

/*自定义通用函数类型*/
typedef void thread_func(void*);

/*进程线程状态*/
enum task_status{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

/*中断栈*/
struct intr_stack{
	uint32_t vec_no;		/*kernel.S 中中断宏的中断号*/
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;		/*无用,pushad*/
	uint32_t ebx;			
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	/*通用寄存器*/
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	/*cpu从低特权级到高特权级时候压入*/
	uint32_t err_code;
	void (*eip)(void);
	uint32_t cs;
	uint32_t eflags;
	void *esp;
	uint32_t ss;
	
};

/* 线程栈 */
struct thread_stack{
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;

	/* 线程第一次执行,eip指向kernel_thread，其他时候指向switch_to返回地址 */
	void (*eip)(thread_func* func, void* func_arg);

	/* 以下是第一次被cpu调度使用 */
	void (*unused_retaddr);
	thread_func* function;
	void* func_arg;
};

/*进程控制块pcb*/
struct task_struct{
	uint32_t* self_kstack;		/* 内核线程 */
	enum task_status status;
	uint8_t priority;
	char name[16];
	uint32_t stack_magic;	/* 魔数, 检测栈溢出 */
};

void thread_create(struct task_struct * pthread, thread_func function, void * func_arg);
struct task_struct* thread_start(char * name, int prio, thread_func * function, void* func_arg);
void init_thread(struct task_struct * pthread, char * name, int prio);
#endif