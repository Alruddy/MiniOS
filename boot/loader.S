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
			
	; totol_mem_bytes 用于保存内存容量，以字节为单位
	; 当前偏移loader.bin文件头0x203字节
	; loader.bin的加载地址为0x900
	; 所以total_mem_bytes内存中的地址是0xb03
	total_mem_bytes dd 0
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	

	; gdt 指针， 前两个字节是gdt界限， 后4字节是起始地址
	gdt_ptr dw GDT_LIMIT
			dd GDT_BASE
	
	; 人工对齐： 3 + total_mem_bytes4 + gdt_ptr6 + ards_buff232 + ards_nr2 + error_hlt9 共256字节
	ards_buff times 232 db 0
	ards_nr dw 0		; ards 结构数量
	
	loadermsg db 'error_hlt'
	; loader_start 地址是0xc00
	loader_start:
	
	; int 0x15 eax=0xe820, edx=0x534d4150 'SMAP' 获取内存布局

	xor ebx, ebx		; ebx = 0
	mov edx, 0x534d4150	; edx只赋值一次
	mov di, ards_buff	; ards 结构缓冲区
.e820_mem_get_loop:
	mov eax, 0x0000e820 ; 每次执行后都会被bios重新赋值，所以要重新赋值
	mov ecx, 20			; ards 结构写入的最小大小为20字节
	int 0x15			; 调用bios终端
	jc .e820_failed_so_try_e801   ; 如果cf位为1则进行跳转, 如果cf为1，那么就尝试 0xe801子功能
	add di, cx			; 指向新的缓冲区
	inc word [ards_nr]  ; 增加记录数
	cmp ebx, 0			; ebx=0, cf=0 读取内存信息完成, ards全部返回
	jnz .e820_mem_get_loop ; 不是最后一个进行跳转
	
; 在所有的ards结构中，找出(base_add_low + length_low) 的最大值，是内存的容量
	mov cx, [ards_nr]	; 遍历ards结构体次数
	mov ebx, ards_buff
	xor edx, edx 		; edx为最大内存容量, 先清0

.find_max_mem_area:
	mov eax, [ebx]		; base_add_low
	add eax, [ebx+8]	; length_low
	add ebx, 20			; 下一个ards缓冲区
	cmp edx, eax		; 比较最大，edx为最大值
	jge	.next_ards
	mov edx, eax
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok		; 输出大小

; --------- int 0x15; ax = 0xe801 获取内存大小，最大4G --------
; 返回后, ax, cx 一样以KB为单位； bx, dx一样以64KB为单位
; ax, cx 为0~15M的内容, bx, dx 为16M~4G的内容
.e820_failed_so_try_e801:
	mov ax, 0xe801
	int 0x15
	jc .e801_failed_so_try_88

; 先算出低16M的内存数量，转换为以byte为单位
	mov cx, 0x400		; 单位为1KB
	mul cx				; 16位乘法，dx储存高16位，ax储存低16位
	shl edx, 16
	and eax, 0x0000ffff
	or  edx, eax		; (dx<<16) + ax
	add edx, 0x100000 	; edx + 1M
	mov esi, edx 		; 备份edx
	
; 算出16~4G的内存数量， 转化位byte为单位
	xor eax, eax		; eax 清0
	mov ax, bx			; eax = bx 
	mov ecx, 0x40000	; 单位为64KB
	mul ecx				; 32位乘法， edx存放高32位, eax存放低32位
	add esi, eax
; 因为4G-1可以用32位来表示，所以32位够用了
	mov edx, esi		; edx 是内存大小
	jmp .mem_get_ok
	
; ------------- int 0x15 ; ah = 0x88 获取内存大小， 最大64M --------
.e801_failed_so_try_88:
	mov ah, 0x88		; 返回结果在ax中
	int 0x15
	jc 	.error_hlt
	and eax, 0x0000ffff
; 算出内存大小， 转化为单位为byte
; 当前单位为1KB
	mov cx, 0x400
	mul cx
	shl edx, 16
	and eax, 0x0000ffff
	add edx, eax
	add edx, 0x100000		; 0x88只会返回1M之上的内存 + 1M
	
.mem_get_ok:
	mov [total_mem_bytes], edx ; 将内存大小存到total_mem_bytes处
    jmp .start_protected_mode
.error_hlt:	
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
.start_protected_mode:
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

