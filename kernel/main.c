#include "print.h"
#include "init.h"
#include "debug.h"
#include "thread.h"

void k_thread_a(void*);

int main(void) {
	put_str("I'm kernel\n");
	init_all();

	thread_start("k_thread_a", 31, k_thread_a, "arg A");

	while(1);
	return 0;
}

void k_thread_a(void* arg) {
	char *para = arg;
	while(1) put_str(para);
}