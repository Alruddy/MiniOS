#include "print.h"
int main() {
	put_str("I'm kernel print_str program!\n");
	put_int(0x12345678);
	put_char('\n');
	put_int(0x00001234);
	while(1);
	return 0;
}
