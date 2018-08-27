/**************机器模式****************
	b -- 输出寄存器的QImode 表示寄存器低8位  [a-d]l
	w -- 输出寄存器的HImode 表示寄存器低16位 [a-d]x
	HImode -- Half Integer Mode 		1/2
	QImode -- Quater Integer Mode		1/4
***************************************/
#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"

/*向端口写入一个字节*/
static inline void outb(uint16_t port, uint8_t data) {
/*	对端口N表示0~255的立即数, d表示dx*/
	asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(data));
}

/* 将addr处开始的word_cnt个字写入端口 */
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
/* 	+ 表示既作为读入又作为写入 
	outsw 是将 ds:esi 的16位内容写入port 
*/
	asm volatile("cld; rep outsw" : "+S"(addr), "+c"(word_cnt) : "d"(port));
}

/* 从端口读取一个字节 */
static inline uint8_t inb(uint16_t port) {
	uint8_t data;
	asm volatile("inb %w1, %b0" : "=a"(data) : "Nd"(port));
	return data;
}

/* 从端口读取word_cnt个字到addr */
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt) {
/*insw将从端口port读取16位内容写入到es:edi*/
	asm volatile("cld; rep insw" : "+D"(addr), "+c"(word_cnt) : "d"(port) : "memory");
}
#endif

