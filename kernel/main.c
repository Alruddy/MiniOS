#include "print.h"
#include "init.h"
#include "debug.h"
int main(void) {
	put_str("I'm kernel\n");
	init_all();

	void* vaddr = get_kernel_pages(3);
	put_str("\n get_kernel_page start vaddr: ");
	put_int((uint32_t)vaddr);
	put_str("\n");

	while(1);
	return 0;
}
