#include "print.h"
#include "init.h"
#include "debug.h"
int main(void) {
	put_str("I'm kernel\n");
	init_all();
	//asm volatile("sti"); // 打开IF, 开中断
	ASSERT(1 == 2);
	while(1);
	return 0;
}
