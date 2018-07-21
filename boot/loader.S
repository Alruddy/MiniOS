%include "boot.inc"
SECTION LOADER vstart=LOADER_BASE_ADDR
; 输出字符串
	mov byte [gs:0x00], '2'
	mov byte [gs:0x01], 0xa4

	mov byte [gs:0x02], ' '
	mov byte [gs:0x03], 0xa4

	mov byte [gs:0x04], 'l'
	mov byte [gs:0x05], 0xa4
	mov byte [gs:0x06], 'o'
	mov byte [gs:0x07], 0xa4
	mov byte [gs:0x08], 'a'
	mov byte [gs:0x09], 0xa4
	mov byte [gs:0x0a], 'd'
	mov byte [gs:0x0b], 0xa4
	mov byte [gs:0x0c], 'e'
	mov byte [gs:0x0d], 0xa4
	mov byte [gs:0x0e], 'r'
	mov byte [gs:0x0f], 0xa4

	jmp $
