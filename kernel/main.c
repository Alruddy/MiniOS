#include "print.h"
#include "init.h"
#include "debug.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "keyboard.h"

void k_thread_a(void*);
void k_thread_b(void*);
int main(void) {
	put_str("I'm kernel\n");
	init_all();

	thread_start("k_thread_a", 31, k_thread_a, "A_ ");
	thread_start("k_thread_b", 31, k_thread_b,  "B_ ");
	intr_enable();
	while(1) {
//		console_put_str("Main ");
	}
	return 0;
}

void k_thread_a(void* arg) {
	char *para = arg;
	while(1) {
		enum intr_status old_status = intr_disable();
		if (!ioq_empty(&kbd_buf)) {
			console_put_str(para);
			char byte = ioq_getchar(&kbd_buf);
			console_put_char(byte);
		}
		intr_set_status(old_status);
	}
}

void k_thread_b(void *arg) {
	char *para = arg;
	while(1) {
		enum intr_status old_status = intr_disable();
		if (!ioq_empty(&kbd_buf)) {
			console_put_str(para);
			char byte = ioq_getchar(&kbd_buf);
			console_put_char(byte);
		}
		intr_set_status(old_status);
	}
}