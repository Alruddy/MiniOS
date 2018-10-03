#include "init.h"
#include "stdint.h"
#include "interrupt.h"
#include "timer.h"
#include "print.h"
#include "memory.h"

/*负责初始化所有模块*/
void init_all(void) {
	put_str("init_all\n");
	idt_init(); // 初始化中断
	timer_init();
	mem_init();
	thread_init();
}

