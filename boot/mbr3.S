; 主引导程序
; ---------------------------------
%include "boot.inc"
SECTION MBR vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov sp, 0x7c00
	mov ax, 0xb800
	mov gs, ax
; 清屏
; 使用6号功能号，上卷所有行
; ---------------------------
; 输入： 
; ah 功能号
; al 上卷行数 0为所有行
; bh 上卷行属性
; cl, ch 窗口左上角 x,y
; dl, dh 窗口右下角 x,y
; 无返回值
	mov ax, 0x0600
	mov bx, 0x0700
	mov cx, 0x0000
	mov dx, 0x184f
	
	int 0x10
; 输出字符串
; -------------------

	mov byte [gs:0x00], '1'
	mov byte [gs:0x01], 0xa4
	
	mov byte [gs:0x02], ' '
	mov byte [gs:0x03], 0xa4

	mov byte [gs:0x04], 'M'
	mov byte [gs:0x05], 0xa4

	mov byte [gs:0x06], 'B'
	mov byte [gs:0x07], 0xa4

	mov byte [gs:0x08], 'R'
	mov byte [gs:0x09], 0xa4

; ---------------------------------
	mov eax,LOADER_START_SECTOR ; 起始扇区lba地址
	mov bx, LOADER_BASE_ADDR	; 写入的地址
	mov cx, 1					; 写入的扇区数量
	call rd_disk_m_16			; 一下读取扇区程序的起始部分 
	jmp LOADER_BASE_ADDR
; ---------------------------------
; 功能： 读取某一个扇区
rd_disk_m_16:
; ---------------------------------

	mov esi, eax				; 备份初始扇区地址
	mov di, cx					; 备份数量

; 读写硬盘
; 1. 设置扇区数量
	mov dx, 0x1f2
	mov al, cl
	out dx, al 					; 读取扇区数

	mov eax, esi
; 2. 设置LBA 地址
	mov dx, 0x1f3				; 0~7
	out dx, al

	mov cl, 8
	shr eax, cl
	mov dx, 0x1f4				; 8~15
	out dx, al
	
	shr eax, cl	
	mov dx, 0x1f5				; 16~23
	out dx, al

	shr eax, cl					; 24~27
	and al, 0x0f
	or  al, 0xe0
	mov dx, 0x1f6
	out dx, al

; 3. 端口写入读命令
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

; 4. 检测硬盘状态
 .not_ready:
	; 统一端口， 读时表示状态
	nop
	in al, dx
	and al, 0x88 ; 0x80 硬盘忙 0x08 硬盘准备好
	cmp al, 0x08
	jnz .not_ready
; 5. 从数据端口读取数据
	mov ax, di
	mov dx, 256
	mul dx
	mov cx, ax
	; di 是扇区数量, 一个扇区512字节，每次读取一个字，所以是*256
	mov dx, 0x1f0
.go_on_read:
	in ax, dx
	mov [bx], ax
	add bx, 2
	loop .go_on_read
	ret

	times 510-($-$$) db 0
	db 0x55, 0xaa

