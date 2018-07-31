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
	
	call setup_page ; 设置页目录和页表
	; 将视频显示段基址加上0xc0000000
	sgdt [gdt_ptr]	
	mov ebx, [gdt_ptr+2]
	or dword [ebx + 0x18 + 4], 0xc0000000
	
	; gdt基址加上0xc0000000成为内核地址
	add dword [gdt_ptr+2], 0xc0000000
	; 栈指针移动到内核区域
	add esp, 0xc0000000
	; 页目录赋值为cr3
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax
	; 打开cr0的pg位，设置分页
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	; 重新加载gdt
	lgdt [gdt_ptr]
	; 使用新地址打印字符
	mov byte [gs:162], 'V'
	
	jmp $

; -------------------- 创建页目录和页表 ------------------------
setup_page:
; 先将页目录的占用空间逐字节清0
	mov ecx, 4096
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

; 开始创建页目录项PDE
.create_pde:
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000		; 第一个页表项的地址
	mov ebx, eax		; 储存基址
; 将页目录项中0和0xc00都指向第一个页表项地址

; 设置目录项的属性RW和P都为1， US为1表示用户属性，所有特权级别都可以访问
	or eax, PG_RW_W | PG_P | PG_US_U
	mov [PAGE_DIR_TABLE_POS + 0x000], eax	; 最多有1024个页目录项，最后一个目录项指向自己目录表地址
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax	; 768
	
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax	; 1023
	
; 创建页表 pte
	mov ecx, 256	; 因为1M/4k = 256
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
	mov [ebx + 4*esi], edx
	add edx, 4096
	inc esi
	loop .create_pte
	
; 创建内核和其他页表项pde, 769~1022
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000	; 第二个页表项
	or eax,  PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254
	mov esi, 769
.create_kernel_pde:
	mov [ebx + esi*4], eax
	inc esi
	add eax, 0x1000
	loop .create_kernel_pde
	ret
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	