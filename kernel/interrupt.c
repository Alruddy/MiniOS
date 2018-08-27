#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"

#define PIC_M_CTRL 0x20			// 主片控制口 icw1/ocw2/ocw3
#define PIC_M_DATA 0x21			// 主片数据口 icw2/icw3/icw4/ocw1
#define PIC_S_CTRL 0xa0			// 从片控制口
#define PIC_S_DATA 0xa1			// 从片数据口

#define IDT_DESC_CNT 0x21 		// 中断向量数

/*中断门描述符结构*/
struct gate_desc {
	uint16_t	func_offset_low_word;		// 中断处理程序偏移地址低8位
	uint16_t	selector;					// 中断处理程序段选择子
	uint8_t		dcount;						// 固定值， 中断门的第4字节
	uint8_t		attribute;					// 中断门的属性
	uint16_t 	func_offset_high_word;		// 中断处理程序偏移地址高8位
};

// 静态函数声明
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];			// idt是中断描述符表,是描述符数组
extern intr_handler intr_entry_table[IDT_DESC_CNT];	// 引用kernel.S中的描述符数组

/*创建中断门描述符*/
static void make_idt_desc(struct gate_desc * p_gdesc, uint8_t attr, intr_handler function) {
	p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
	p_gdesc->selector = SELECTOR_K_CODE;
	p_gdesc->dcount = 0;
	p_gdesc->attribute = attr;
	p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;
}

/*初始化中断描述符表*/
static void idt_desc_init(void) {
	int i;
	for (i = 0; i < IDT_DESC_CNT; i++) {
		make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
	}
	put_str("    idt_desc_init done\n");
}

/*初始化可编程中断控制器*/
static void pic_init(void) {
	/*初始化主片*/
	outb(PIC_M_CTRL, 0x11);		// ICW1: 边沿触发，级联8259，需要ICW4
	outb(PIC_M_DATA, 0x20);		// ICW2: 起始中断向量号位0x20
	outb(PIC_M_DATA, 0x04);		// ICW3: IR2接从片
	outb(PIC_M_DATA, 0x01);		// ICW4: 8086模式，正常EOI

	/*初始化从片*/
	outb(PIC_S_CTRL, 0x11);
	outb(PIC_S_DATA, 0x28);		// ICW2: 起始中断向量号位0x28
	outb(PIC_S_DATA, 0x02);		// ICW3: 接在主片IR2
	outb(PIC_S_DATA, 0x01);

	/*设置中断屏蔽,打开主片上的时钟产生的中断 ir0*/
	outb(PIC_M_CTRL, 0xfe);
	outb(PIC_S_CTRL, 0xff);
	put_str("    pic_init done\n");
}


/*完成有关中断的所有初始化工作*/
void idt_init() {
	put_str("idt_init start\n");
	idt_desc_init();	// 初始化描述符
	pic_init();			// 初始化8259A

	/*加载idt*/
	uint64_t idt_operand = ((sizeof(idt)-1) | ((uint64_t)((uint32_t)idt) << 16)) ;
	asm volatile("lidt %0" : : "m"(idt_operand));
	put_char("idt_init done\n");
}


