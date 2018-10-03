[bits 32]
section .text
global switch_to
switch_to:
	; 返回地址
	push esi
	push edi
	push ebx
	push ebp
	; 上下文保护需要保护的4个寄存器

	mov eax, [esp+20] 	; cur
	mov [eax], esp		; 保存当前栈的esp指针, 赋值给self_kstack

	; 备份cur环境, 恢复next环境
	mov eax, [esp+24]	; next
	mov esp, [eax]

	pop ebp
	pop ebx
	pop edi
	pop esi
	ret
