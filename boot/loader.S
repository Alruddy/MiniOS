    %include "boot.inc"
    SECTION loader vstart=LOADER_BASE_ADDR
    LOADER_STACK_TOP equ LOADER_BASE_ADDR
    jmp loader_start
    
; ------- 构建GDT和内部描述符 ---------

    GDT_BASE: 	dd 0x00000000
				dd 0x00000000
	CODE_DESC: 	dd 0x0000ffff
				dd DESC_CODE_HIGH4
	DATA_STACK_DESC:	dd 0x0000ffff
						dd DESC_DATA_HIGH4
	VIDEO_DESC:	dd 0x80000007 ; limit = (bffff - b8000) / 4k = 7
				dd DESC_VIDEO_HIGH4
	
	GDT_SIZE 	equ $ - GDT_BASE
	GDT_LIMIT 	equ GDT_SIZE - 1
	times  60 dq 0 ; 60个描述符空位 ; db 1; dw 2; dd 4; dq 8;
	
	SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0
	; (CODE_DESC - GDT_BASE) / 8
	SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
	SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0
	
	; gdt 指针， 前两个字节是gdt界限， 后4字节是起始地址
	gdt_ptr dw GDT_LIMIT
			dd GDT_BASE
	
	; 加载的字符串
	loadermsg db '2 loader in real.'
	
	loader_start:
; ----------------------------------------------------	
; --------------- 使用BIOS中断打印字符串 -------------
; ----------------------------------------------------
; int 0x10 ; 功能号 0x13
; 输入： ah 功能号 0x13
; bh 页码   bl 属性
; cx 字符串长度
; dl, dh  字符串的 列,行
; es:bp 字符串地址
; al 输出方式：
; 字符串只含有字符,属性在BL中: 	0 含显示字符 光标位置不变 ; 1 显示字符， 光标位置改变
; 字符串含有字符和属性:  		3 显示字符和属性 光标不变; 4 显示字符和属性 光标改变
; 无返回值
	mov sp, LOADER_BASE_ADDR
	mov bp, loadermsg
	mov cx, 17
	mov ax, 0x1301
	mov bx, 0x001f
	mov dx, 0x1800
	
	int 0x10
; ------------- 进入保护模式 -------------
; 1. 打开A20
; 2. 加载gdt
; 3. 将cr0的pe位置1

; --------------- 打开A20 ----------------
	in al, 0x92
	or al, 0000_0010b
	out 0x92, al
	
; --------------- 加载gdt ----------------
	lgdt [gdt_ptr]
	
; --------------- 将cr0的pe(0)位置1
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

	jmp dword SELECTOR_CODE:p_mode_start ; 刷新流水线

; 保护模式的操作位数
[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax
	
	mov byte [gs:160], 'P'
    mov byte [gs:161], 0xa4	
	jmp $
