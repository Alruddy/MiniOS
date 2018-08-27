#include "print.h"
#include "init.h"
int main() {
	put_str("I'm kernel\n");
	init_all();
	asm volatile("sti"); // 打开IF, 开中断
	while(1);
	return 0;
}
