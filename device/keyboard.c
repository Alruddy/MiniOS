#include "keyboard.h"
#include "io.h"
#include "print.h"
#include "interrupt.h"
#include "global.h"

#define KBD_BUF_PORT 0x60  /* 键盘寄存器端口号 */

static void intr_keyboard_handler(void) {
	put_char('k');
	inb(KBD_BUF_PORT);
	return;
}

/* 键盘初始化 */
void keyboard_init() {
	put_str("keyboard init start.\n");
	register_handler(0x21, intr_keyboard_handler);
	put_str("keyboard init done.\n");
}

