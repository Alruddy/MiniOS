#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define BUFSIZE 64
/* 环形队列 */
struct ioqueue{
	/* 生产者消费者问题 */
	struct lock lock;				/*  */
	struct task_struct* producer;	/* 生产者 */
	struct task_struct* consumer;	/* 消费者 */
	char buf[BUFSIZE];
	uint32_t head;					/* 队首, 数据写入区 */
	uint32_t tail;					/* 队尾, 数据读取区 */
};

bool ioq_empty(struct ioqueue * ioq);
bool ioq_full(struct ioqueue * ioq);
void ioqueue_init(struct ioqueue * ioq);
void ioq_putchar(struct ioqueue * ioq, char byte);
char ioq_getchar(struct ioqueue * ioq);

#endif