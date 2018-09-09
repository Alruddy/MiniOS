#include "interrupt.h"
#include "debug.h"
#include "print.h"

/*打印文件名, 行号, 函数名, 条件*/
void panic_spin(char * filename, int line, const char * func, const char * condition) {
	intr_disable(); // 关中断
	put_str("\n\n\n!!!!!!!error!!!!!!!!!!\n");
	put_str("filename: "); put_str(filename); put_str("\n");
	put_str("line: "); put_int(line); put_str("\n");
	put_str("func: "); put_str(func); put_str("\n");
	put_str("condition: "); put_str(condition); put_str("\n");
	while(1);
}