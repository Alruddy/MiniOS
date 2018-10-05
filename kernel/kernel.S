[bits 32]
%define ERROR_CODE nop	; 若在相关异常已经压入错误码，空操作来保持栈格式统一
%define ZERO push 0	; 若在相关异常没有压入错误码，压入一个0来保持栈格式统一

extern put_str		; 声明外部函数
extern idt_table	; C中的中断处理函数

section .data 
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table		;中断向量入口表
intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry:			; 每个中断处理的向量所对应的处理函数
	%2		 	; ERROR_CODE / ZERO

	; 保存上下文
	push ds
	push es
	push fs
	push gs
	pushad	; 压入8个32位寄存器 eax, ecx, edx, edx, esp, ebp, esi, edi

	; 如果从片中进入的中断
	; 除了王从片发送EOI，还要往主片上发送
	mov al, 0x20
	out 0xa0, al 
	out 0x20, al 

	push %1
	call [idt_table + %1*4]	; 调用C中断处理函数

	jmp intr_exit		; 退出

section .data 
	dd intr%1entry		; intr_entry_table数组 
%endmacro

section .text
global intr_exit
intr_exit:
	; 恢复上下文环境
	add esp, 4
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 4 ; 跳过error_code
	iretd
	

VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO
VECTOR 0x08,ZERO
VECTOR 0x09,ZERO
VECTOR 0x0a,ZERO
VECTOR 0x0b,ZERO
VECTOR 0x0c,ZERO
VECTOR 0x0d,ZERO
VECTOR 0x0e,ZERO
VECTOR 0x0f,ZERO
VECTOR 0x10,ZERO
VECTOR 0x11,ZERO
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO
VECTOR 0x18,ZERO
VECTOR 0x19,ZERO
VECTOR 0x1a,ZERO
VECTOR 0x1b,ZERO
VECTOR 0x1c,ZERO
VECTOR 0x1d,ZERO
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO
VECTOR 0x20,ZERO ; 时钟
VECTOR 0x21,ZERO ; 键盘
VECTOR 0x22,ZERO ; 级联
VECTOR 0x23,ZERO ; 串口2
VECTOR 0x24,ZERO ; 串口1
VECTOR 0x25,ZERO ; 并口2
VECTOR 0x26,ZERO ; 软盘入口
VECTOR 0x27,ZERO ; 并口1
VECTOR 0x28,ZERO ; 实时时钟
VECTOR 0x29,ZERO ; 重定向
VECTOR 0x2a,ZERO ; 
VECTOR 0x2b,ZERO ;
VECTOR 0x2c,ZERO ; ps/2鼠标
VECTOR 0x2d,ZERO ; fpu浮点单元
VECTOR 0x2e,ZERO ; 硬盘
VECTOR 0x2f,ZERO ;

