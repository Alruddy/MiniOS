#include "init.h"
#include "stdint.h"
#include "interrupt.h"
#include "timer.h"
#include "print.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"

/*负责初始化所有模块*/
void init_all(void) {
	put_str("init_all\n");
	idt_init(); 		/* 初始化中断 */
	mem_init();			/* 初始化内存管理 */
	thread_init();		/* 初始化线程结构 */
	timer_init();		/* 初始化PIT计时器 */
	console_init();		/* 初始化终端 */
	keyboard_init(); 	/* 键盘初始化 */
}

