#include "timer.h"
#include "io.h"
#include "stdint.h"
#include "print.h"
#include "thread.h"
#include "interrupt.h"
#include "debug.h"

#define IRQ0_FREQUENCY		100
#define INPUT_FREQUENCY 	1193180
#define COUNTER0_VALUE		INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT 		0x40
#define COUNTER0_NO			0
#define COUNTER0_MODE		2
#define READ_WRITE_LATCH	3
#define PIT_CONTROL_PORT 	0x43

uint32_t ticks;		/* 内核中断开启以来的滴答数 */

/*把操作计数器counter_no, 读写锁属性rwl, 计数器模式counter_mode,
写入模式控制寄存器, 并初始化计数初值*/
static void frequency_set(uint8_t counter_port, 
						  uint8_t counter_no,
						  uint8_t rwl,
						  uint8_t counter_mode,
						  uint16_t counter_value){
/*往控制字寄存器端口0x43中写入控制字*/
	outb(PIT_CONTROL_PORT, (uint8_t)((counter_no << 6) | (rwl << 4) | (counter_mode << 1)));
/*先写入第八位*/
	outb(counter_port, (uint8_t)(counter_value));
/*再写入高8位*/
	outb(counter_port, (uint8_t)(counter_value >> 8));
}


/* 时钟中断处理函数 */
static void intr_timer_handler(void) {
	struct task_struct* cur_thread = running_thread();
	ASSERT(cur_thread->stack_magic == 0x12345678);  /* 检查栈是否溢出 */
	cur_thread->elapsed_ticks++;	/* 内核处理时间 */
	ticks++;	/* 内核时间 */
	if (cur_thread->ticks == 0) {
		schedule();		// 时间片用完,调度
	} else {
		cur_thread->ticks--;
	}
}

					  
/*初始化PIT8253*/
void timer_init() {
	put_str("timer_init start.\n");
	frequency_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER0_MODE, COUNTER0_VALUE);
	register_handler(0x20, intr_timer_handler);
	put_str("timer_init done.\n");
}

