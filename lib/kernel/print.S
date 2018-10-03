TI_GDT equ 000b
RPL0 equ 00b
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

section .data 
put_int_buffer dq 0

[bits 32]
section .text
; --------------------- put_char --------------
; 把栈中的第一个字符打印到光标处
; ---------------------------------------------
global put_char
put_char:
	pushad		; 备份32位寄存器环境
	; eax, ecx, edx, ebx, esp, ebp, esi, edi
	; 需要保证gs中的为正确的视频段选择子
	; 每次打印都为gs赋值
	mov ax, SELECTOR_VIDEO
	mov gs, ax

; 获取光标当前位置
	mov dx, 0x03d4
	mov al, 0x0e	; 光标位置高8位
	out dx, al 
	mov dx, 0x03d5
	in  al, dx
	mov ah, al 

	mov dx, 0x03d4
	mov al, 0x0f	; 光标位置低8位
	out dx, al 
	mov dx, 0x03d5
	in  al, dx

; 将光标位置存入bx
	mov bx, ax
; 从栈中获取打印字符
	mov ecx, [esp+36] ; pushad 压入8个寄存器，加上主调函数的4字节返回地址，共4*8+4=36B
; 考虑特殊字符
	cmp cl, 0xd	; 回车CR
	jz .is_carriage_return
	cmp cl, 0xa	; 换行LF
	jz .is_line_feed

	cmp cl, 0x8	; backspace 
	jz .is_backspace

	jmp .put_other
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.is_backspace:
	dec bx		; bx--
	shl bx, 1	; bx *= 2

	mov byte [gs:bx], 0x20	;space
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1	; bx /= 2
	jmp .set_cursor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.put_other:
	shl bx, 1

	mov byte [gs:bx], cl
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	inc bx			; 光标位置加一
	cmp bx, 2000 
	jl .set_cursor		; 少于最大界限

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.is_line_feed:
.is_carriage_return:
	xor dx, dx	; dx = 0
	mov ax, bx	; ax = bx
	mov si, 80	; si = 80

	div si		; dx = ax % si ; ax = ax / si
	sub bx, dx	; bx -= dx 回到行首

.is_carriage_return_end:
	add bx, 80	; 下一行
	cmp bx, 2000	; 是否超出显示范围2000=80*25
.is_line_feed_end:
	jl .set_cursor

; 屏幕行范围是0~24,滚屏将1~24搬到0~23
.roll_screen:
	cld
	mov ecx, 960 	; 4000 - 80*2 = 4000-160 = 3840
			; 3840 / 4 = 960
	mov esi, 0xc00b80a0	; 内核地址0xc0000000
	mov edi, 0xc00b8000	
	rep movsd

	; 最后一行清空
	mov ebx, 3840
	mov ecx, 80
.cls:
	mov word [gs:ebx], 0x0720 	; 小端 20 07
	add ebx, 2
	loop .cls
	
	mov bx, 1920		;最后一行行首
	
.set_cursor:
	mov dx, 0x03d4
	mov al, 0x0e		; 光标高8位
	out dx, al 
	mov dx, 0x03d5
	mov al, bh
	out dx, al
	
	mov dx, 0x03d4
	mov al, 0x0f		; 光标低8位
	out dx, al 
	mov dx, 0x03d5
	mov al, bl
	out dx, al

.put_char_done:
	popad
	ret
	
; ---------------- set_cursor ------------------------
; 设置光标位置, 接受一个参数(0~1999)
; ---------------------------------------------------
global set_cursor
set_cursor:
	push eax
	push ebx
	push edx
	mov ebx, [esp+16]
.set_cursor_begin:
	mov dx, 0x03d4
	mov al, 0x0e		; 光标高8位
	out dx, al 
	mov dx, 0x03d5
	mov al, bh
	out dx, al 

	mov dx, 0x03d4
	mov al, 0x0f		; 光标低8位
	out dx, al 
	mov dx, 0x03d5
	mov al, bl
	out dx, al
.set_cursor_done:
	pop edx
	pop ebx
	pop eax
	ret
; ---------------- print_str -------------------------
; 通过put_char来打印字符串 cdecl调用约定
; ----------------------------------------------------
global put_str
put_str:
	push ebx
	push ecx
	; 字符串首地址和字符串长度
	xor ecx, ecx
	mov ebx, [esp+12] 	; 从栈中获得首地址
.goon:
	mov cl, [ebx]
	cmp cl, 0	;字符串结尾
	jz  .str_over
	push ecx	; 压入打印字符
	call put_char
	add esp, 4	; cdecl 约定
	inc ebx		; 下一个字符
	jmp .goon
.str_over:
	pop ecx
	pop ebx
	ret

; -------------------- put_int -----------
; 将小端字节序的数字变成ASCII码后，倒置
; -----------------------------------------

global put_int 
put_int:
	pushad
	mov ebp, esp
	mov eax, [ebp+36]	; 获取栈中的整数
	mov edx, eax
	mov edi, 7		; 储存ASCII码
	mov ecx, 8		; 8个字符
	mov ebx, put_int_buffer

; 从低到高处理每4个bit
.16based_4bits:
	and edx, 0x0000000f
	cmp edx, 9
	jg .is_A2F
	add edx, '0'	; edx = edx + '0'
	jmp .store
.is_A2F:
	sub edx, 10	; edx >= 10
	add edx, 'A'	; edx = edx - 10 + 'A'
.store:
	mov [ebx+edi], dl
	dec edi
	shr eax, 4
	mov edx, eax
	loop .16based_4bits
	
.ready_to_print:
	inc edi		;edi = 0
.skip_prefix_0:
	cmp edi, 8	; 最后一个字符
	je .full0
.go_on_skip:
	mov ecx, [ebx+edi]
	inc edi
	cmp cl, '0'
	je  .skip_prefix_0
	dec edi
	jmp .put_each_num

.full0:
	mov cl, '0'
.put_each_num:
	push ecx	;压入参数
	call put_char
	add esp, 4
	inc edi
	mov cl, [ebx+edi]
	cmp edi, 8
	jl .put_each_num
	popad
	ret


