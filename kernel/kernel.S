[bits 32]
%define ERROR_CODE nop	; 若在相关异常已经压入错误码，空操作来保持栈格式统一
%define ZERO push 0	; 若在相关异常没有压入错误码，压入一个0来保持栈格式统一

extern put_str		; 声明外部函数

section .data 
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table		;中断向量入口表
intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry:			; 每个中断处理的向量所对应的处理函数
	%2		 	; ERROR_CODE / ZERO
	push intr_str
	call put_str
	add esp, 4		; 调用约定

	; 如果是从片上进入的中断，要向主从的8425A发送EOI
	mov al, 0x20
	out 0x20, al 
	out 0xa0, al 

	add esp, 4		; 调用约定,弹出地址
	iret

section .data 
	dd intr%1entry		; intr_entry_table数组 
%endmacro

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
VECTOR 0x20,ZERO
