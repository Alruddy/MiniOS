#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define PIC_M_CTRL 0x20			// 主片控制口 icw1/ocw2/ocw3
#define PIC_M_DATA 0x21			// 主片数据口 icw2/icw3/icw4/ocw1
#define PIC_S_CTRL 0xa0			// 从片控制口
#define PIC_S_DATA 0xa1			// 从片数据口

#define IDT_DESC_CNT 0x21 		// 中断向量数
#define EFLAGS_IF	0x00000200	// eflags 中的if位置1
#define GET_EFLAGS(EFLAG_VAR)  asm volatile ("pushfl; popl %0" : "=g"(EFLAG_VAR))

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
char* intr_name[IDT_DESC_CNT];						// 异常处理
intr_handler idt_table[IDT_DESC_CNT];				// 中断处理函数数组,在kernel.S的入口中调用
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
	outb(PIC_M_DATA, 0xfe);
	outb(PIC_S_DATA, 0xff);
	put_str("    pic_init done\n");
}

/*通用的中断处理函数，用于中断异常出现的处理*/
static void general_intr_handler(uint8_t vec_nr) {
	if (vec_nr == 0x27 || vec_nr == 0x2f) {
		// 伪中断
		return;
	}
	put_str("int vector : 0x");
	put_int(vec_nr);
	put_char('\n');
}


/*完成一般中断处理函数注册以及异常名称注册*/
static void exception_init(void) {
	int i;
	for (i = 0; i < IDT_DESC_CNT; i++) {
		/*idt_table 数组中的函数是在进入中断后根据中断向量号调用的*/
		idt_table[i] = general_intr_handler;
		intr_name[i] = "unknown";
	}
	intr_name[0] = "#DE Divide Error"; 
	intr_name[1] = "#DB Debug Exception";
	intr_name[2] = "NMI Interrupt";
	intr_name[3] = "#BP Breakpoint Exception";
	intr_name[4] = "#OF Overflow Exception";
	intr_name[5] = "#BR Bound Range Exceed Exception";
	intr_name[6] = "#UD Invaild Opcode Exception";
	intr_name[7] = "#NM Device Not Available Exception";
	intr_name[8] = "#DF Double Fault Exception";
	intr_name[9] = "Coprocessor Segment Overrun";
	intr_name[10] = "#TS Invaild TSS Exception";
	intr_name[11] = "#NP Segment Not Present";
	intr_name[12] = "#SS Stack Fault Exception";
	intr_name[13] = "#GP General Protection Exception";
	intr_name[14] = "#PF Page-Fault Exception";
	// intr_name[15] intel 保留
	intr_name[16] = "#MF x87 FPU Floating-Point Error";
	intr_name[17] = "#AC Alignment Check Exception";
	intr_name[18] = "#MC Machine-Check Exception";
	intr_name[19] = "#XF SIMD Floating-Point Exception";
}

/*完成有关中断的所有初始化工作*/
void idt_init() {
	put_str("idt_init start\n");
	idt_desc_init();	// 初始化描述符
	exception_init(); 	// 异常名初始化
	pic_init();			// 初始化8259A

	/*加载idt*/
	uint64_t idt_operand = ((sizeof(idt)-1) | ((uint64_t)((uint32_t)idt) << 16)) ;
	asm volatile("lidt %0" : : "m"(idt_operand));
	put_str("idt_init done\n");
}

/*开中断, 并且返回开中断前的状态*/
enum intr_status intr_enable() {
	enum intr_status old_status;
	if (INTR_ON == intr_get_status()) {
		old_status = INTR_ON;
		return old_status;
	} else {
		old_status = INTR_OFF;
		asm volatile("sti");
		return old_status;
	}
}


/*关中断,并且返回关中断前的状态*/
enum intr_status intr_disable(){
	enum intr_status old_status;
	if (INTR_ON == intr_get_status()) {
		old_status = INTR_ON;
		asm volatile("cli": : : "memory"); // 关中断, cli 指令置1
		return old_status;
	} else {
		old_status = INTR_OFF;
		return old_status;
	}
}

/*将中断位置为status*/
enum intr_status intr_set_status(enum intr_status status) {
	return (status & INTR_ON) ? intr_enable() : intr_disable();
}

/*获取中断的中间状态*/
enum intr_status intr_get_status() {
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}


