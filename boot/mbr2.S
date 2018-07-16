;主引导程序
;
; -------------------
SECTION MBR vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov sp, 0x7c00
	mov ax, 0xb800
	mov gs, ax
; 清屏, 利用6功能号，上卷清屏
; ----------------------------------------
; int 0x10 ; 功能号 0x06 ; 功能描述：上卷窗口
; ----------------------------------------
; 输入 ah 功能号 ; al 上卷行数
;  bh 上卷行属性
; cl, ch 窗口左上角 x, y
; dl, dh 窗口右下角 x, y
	mov ax, 0x0600
	mov bx, 0x0700
	mov cx, 0x0000
	mov dx, 0x184f

	int 0x10
; -----------------------------------------

; 输出字符，设置背景绿色，前景红色闪烁
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

	jmp $

; ---------------------------------------

	times 510-($-$$) db 0
	db 0x55, 0xaa











	
