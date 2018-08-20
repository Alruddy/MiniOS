section .data 
str_c_lib: db "c library says: hello world!", 0xa
str_c_lib_len equ $-str_c_lib
str_syscall: db "syscall says: hello world!", 0xa
str_syscall_len equ $-str_syscall

section .text
global _start
_start:
;;;;;;;;;;;;;模拟C库;;;;;;;;
	push str_c_lib_len		; 字符串长度
	push str_c_lib			; 字符串首地址	
	push 1				; 文件描述符

	call simu_write
	add esp, 12
;;;;;;;;;;;;;跨过库函数,直接进行系统调用;;;;;;;
	mov eax, 4
	mov ebx, 1
	mov ecx, str_syscall
	mov edx, str_syscall_len
	int 0x80
;;;;;;;;;;;;;退出;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, 1
	int 0x80

;;;;;;;;自定义simu_write函数;;;;;;;;;;;;;;;;;;;
simu_write:
	push ebp
	mov ebp, esp
	mov eax, 4
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]

	int 0x80
	pop ebp
	ret

	
