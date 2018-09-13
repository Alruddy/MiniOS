#include "string.h"
#include "debug.h"
#include "global.h"

/* 将dst_起始的size个字节置为value */
void memset(void * dst_, uint8_t value, uint32_t size) {
	ASSERT(dst_ != NULL);
	uint8_t* dst = dst_;
	while (size-- > 0) {
		*dst++ = value;
	}
}

/* 将src_的size个字节复制到dst_ */
void memcpy(void * dst_, const void * src_, uint32_t size) {
	ASSERT(dst_ != NULL && src_ != NULL);
	uint8_t* dst = dst_;
	while (size-- > 0) {
		*dst++ = *src_++
	}
}

/* 比较以地址a_和地址b_开头的size字节,      相等返回0,
    a > b 返回 1 
	a < b 返回-1
*/
void memcmp(const void * a_, const void * b_, uint32_t size) {
	const char *a = a_;
	const char *b = b_;
	ASSERT(a != NULL && b != NULL);
	while (size-- > 0) {
		if (*a != *b) {
			return *a > *b ? 1 : -1;
		}
		a++;
		b++;
	}
	return 0;
}

/* 将字符串复制到dst_ */
char* strcpy(char * dst_, const char * src_) {
	ASSERT(dst_ != NULL && src_ != NULL);
	char *r = dst_;
	while((*dst_++ = *src_++)) ;
	return r;
}


/* 比较以地址a_和地址b_开头的size字节,      相等返回0,
    a > b 返回 1 
	a < b 返回-1
*/
char* strcmp(const char * a, const char * b) {
	ASSERT(a != NULL && b != NULL);
	while(*a != 0 && *a == *b) {
		a++;
		b++;
	}
	return *a < *b ? -1 : *a > *b;
}


/* 从左到右查找第一个出现字符的位置 */
char* strchr(const char * str, const uint8_t ch) {
	ASSERT(str != NULL);
	while(*str != 0) {
		if (*str == ch) {
			return (char*) str;
		}
		str++;
	}
	return NULL;
}

/* 从右往左查找第一次出现字符的位置 */
char* strrchr(const char * str, const uint8_t ch) {
	ASSERT(str != NULL);
	const char* last_char = NULL;
	while (*str != 0) {
		if (*str == ch) {
			last_char = str;
		}
		str++;
	}
	return last_char;
}

/*将字符串进行拼接*/
char* strcat(char * dst_, const char * src_) {
	ASSERT(dst_ != NULL && src_ != NULL);
	char* str = dst_;
	while(*str++);
	--str;
	while((*str++ = *src_++));
	return dst_;
}

/* 返回字符出现的次数 */

uint32_t strchrs(const char * str, uint8_t ch) {
	ASSERT(str != NULL);
	uint32_t ch_cnt = 0;
	while (*str != 0) {
		if (*str == ch) {
			ch_cnt++;
		}
	}
	return ch_cnt;
}













