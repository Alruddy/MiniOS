#include "keyboard.h"
#include "io.h"
#include "print.h"
#include "interrupt.h"
#include "global.h"

#define KBD_BUF_PORT 0x60  /* 键盘寄存器端口号 */

/* 用转义字符控制部分字符 */
#define esc 		'\033' 	/* 八进制表示 */
#define backspace	'\b'
#define tab			'\t'
#define enter		'\r'
#define delete		'\x7f' 	/* 十六进制表示 */	

/* 不可见字符均定义为0 */
#define char_invisible 0
#define ctrl_l_char 	char_invisible
#define ctrl_r_char 	char_invisible
#define shift_l_char 	char_invisible
#define shift_r_char	char_invisible
#define alt_l_char		char_invisible
#define alt_r_char		char_invisible
#define caps_lock_char	char_invisible

/* 定义控制字符的通码和断码 */
#define shift_l_make	0x2a
#define shift_r_make	0x36
#define alt_l_make		0x38
#define alt_r_make		0xe038
#define alt_r_break		0xe0b8
#define ctrl_l_make		0x1d
#define ctrl_r_make		0xe01d
#define ctrl_r_break	0xe09d
#define caps_lock_make	0x3a

/* 定义变量来记录控制字符是否被按下 */
/* ext_scancode记录是否是0xe0开头的 */
static bool ctrl_status, alt_status, shift_status, caps_lock_status, ext_scancode;

/* 以通码为索引的二维数组 */
static char keymap[][2] = {
/*扫描码未与shift组合*/
/*-------------------*/
/*0x00*/	{0, 	0},
/*0x01*/	{esc, 	esc},
/*0x02*/	{'1',	'!'},
/*0x03*/	{'2',	'@'},
/*0x04*/	{'3', 	'#'},
/*0x05*/	{'4',	'$'},
/*0x06*/	{'5',	'%'},
/*0x07*/	{'6',	'^'},
/*0x08*/	{'7',	'&'},
/*0x09*/	{'8',	'*'},
/*0x0a*/	{'9',	'('},
/*0x0b*/	{'0',	')'},
/*0x0c*/	{'-',	'_'},
/*0x0d*/	{'=',	'+'},
/*0x0e*/	{backspace,	backspace},
/*0x0f*/	{tab,	tab},
/*0x10*/	{'q',	'Q'},
/*0x11*/	{'w',	'W'},
/*0x12*/	{'e',	'E'},
/*0x13*/	{'r',	'R'},
/*0x14*/	{'t',	'T'},
/*0x15*/	{'y',	'Y'},
/*0x16*/	{'u',	'U'},
/*0x17*/	{'i',	'I'},
/*0x18*/	{'o',	'O'},
/*0x19*/	{'p',	'P'},
/*0x1a*/	{'[',	'{'},
/*0x1b*/	{']',	'}'},
/*0x1c*/	{enter,	enter},
/*0x1d*/	{ctrl_l_char,	ctrl_l_char},
/*0x1e*/	{'a',	'A'},	
/*0x1f*/	{'s',	'S'},
/*0x20*/	{'d',	'D'},	
/*0x21*/	{'f',	'F'},
/*0x22*/	{'g',	'G'},
/*0x23*/	{'h',	'H'},
/*0x24*/	{'j',	'J'},
/*0x25*/	{'k',	'K'},
/*0x26*/	{'l',	'L'},
/*0x27*/	{';',	':'},
/*0x28*/	{'\'',	'"'},
/*0x29*/	{'`',	'~'},
/*0x2a*/	{shift_l_char,	shift_l_char},
/*0x2b*/	{'\\',	'|'},
/*0x2c*/	{'z',	'%'},
/*0x2d*/	{'x',	'%'},
/*0x2e*/	{'c',	'%'},
/*0x2f*/	{'v',	'%'},
/*0x30*/	{'b',	'%'},
/*0x31*/	{'n',	'%'},
/*0x32*/	{'m',	'%'},
/*0x33*/	{',',	'%'},
/*0x34*/	{'.',	'%'},
/*0x35*/	{'/',	'%'},
/*0x36*/	{shift_r_char,	shift_r_char},
/*0x37*/	{'*',	'*'},
/*0x38*/	{alt_l_char,	alt_l_char},
/*0x39*/	{' ',	' '},
/*0x3a*/	{caps_lock_char, 	caps_lock_char}
/* 其他暂不处理 */
};

/* 键盘中断处理函数 */
static void intr_keyboard_handler(void) {
	/* 在这次中断之前的上次中断,先判断控制键是否按下 */
	bool ctrl_down_last = ctrl_status;
	bool shift_down_last = shift_status;
	bool alt_down_last = alt_status;
	bool caps_lock_last = caps_lock_status;

	bool break_code;
	uint16_t scancode = inb(KBD_BUF_PORT);

	/* 若扫描码是e0开头的, 表示此键将产生多个扫描码,  所以结束等待下一个扫描码 */
	if (scancode == 0xe0) {
		ext_scancode = true;	/* 打开e0标记 */
		return;
	}
	/* 如果上次是e0,将扫描码合并 */
	if (ext_scancode) {
		ext_scancode = false;
		scancode = ((0xe000) | scancode);
	}
	break_code = ((scancode & (0x0080)) != 0);
	if (break_code) { /* 如果是断码, 取其通码 */
		uint16_t make_code = (scancode &= (0xff7f));

		/* 如果是控制键弹起了, 改变状态 */
		if (make_code == shift_l_make || make_code == shift_r_make) {
			shift_status = false;
		} else if (make_code == alt_l_make || make_code == alt_r_make) {
			alt_status = false;
		} else if (make_code == ctrl_l_make || make_code == ctrl_r_make) {
			ctrl_status = false;
		}
		return;
	}
	/* 如果是通码,  并且在处理范围之内 */
	else if ((scancode > 0x00 && scancode <= 0x3a) || 
		scancode == alt_r_make || scancode == ctrl_r_make) {
		bool shift = false;
		/* 判断是否与shift组合 */
		if (scancode < 0x0e || scancode == 0x29 ||
			scancode == 0x1a || scancode == 0x1b ||
			scancode == 0x2b || scancode == 0x27 ||
			scancode == 0x27 || scancode == 0x33 ||
			scancode == 0x34 || scancode == 0x35) {
			/*	代表两个字母的键
				0~9-=`[]\;',./
			*/
			if(shift_down_last) {
				shift = true;
			}
		} else { /* 字母键 */
			if(shift_down_last && caps_lock_last) {
				/* shift和caps同时按下 */
				shift = false;
			} else if (shift_down_last || caps_lock_last) {
				/* shift和caps有一个被按下 */
				shift = true;
			} else {
				shift = false;
			}
		}
		uint8_t index = (scancode & 0x00ff);
		char cur_char = keymap[index][shift];
		if (cur_char) { /* 只处理ascii不为0的键 */
			put_char(cur_char);
			return;
		}

		if (scancode == ctrl_l_make || scancode == ctrl_r_make) {
			ctrl_status = true;
		} else if (scancode == alt_l_make || scancode == alt_r_make) {
			alt_status = true;
		} else if (scancode == shift_l_make || scancode == shift_r_make) {
			shift_status = true;
		} else if (scancode == caps_lock_make) {
			caps_lock_status = ! caps_lock_status;
		}
	} 
	else {
		put_str("unknown key.\n");
	}
	return;
}

/* 键盘初始化 */
void keyboard_init() {
	put_str("keyboard init start.\n");
	register_handler(0x21, intr_keyboard_handler);
	put_str("keyboard init done.\n");
}

