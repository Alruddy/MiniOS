#include "ioqueue.h"
#include "debug.h"
#include "global.h"
#include "interrupt.h"

/* 初始化io队列 */
void ioqueue_init(struct ioqueue *ioq) {
	lock_init(&ioq->lock);
	ioq->producer = ioq->consumer = NULL;	/* 生产消费者置空 */
	ioq->head = ioq->tail = 0;				/* 队首队尾指针置0 */
}

/* 返回pos在缓冲区的下一个位置 */
static uint32_t next_pos(uint32_t pos) {
	return (pos+1) % BUFSIZE;
}

/* 判断队列是否已满 */
bool ioq_full(struct ioqueue* ioq) {
	ASSERT(intr_get_status() == INTR_OFF);
	return next_pos(ioq->head) == ioq->tail;
}

/* 判断队列是否已空 */
bool ioq_empty(struct ioqueue* ioq) {
	ASSERT(intr_get_status() == INTR_OFF);
	return ioq->head == ioq->tail;
}

/* 使当前的生产者或者消费者在此缓冲区上等待 */
static void ioq_wait(struct task_struct** waiter) {
	ASSERT(*waiter == NULL && waiter != NULL);
	*waiter = running_thread();
	thread_block(TASK_BLOCKED);
}

/* 唤醒waietr */
static void wakeup(struct task_struct** waiter) {
	ASSERT(*waiter != NULL);
	thread_unblock(*waiter);
	*waiter = NULL;
}

/* 消费者从io队列中获取一个字符 */
char ioq_getchar(struct ioqueue* ioq) {
	ASSERT(intr_get_status() == INTR_OFF);
	/* 若缓冲区为空, 那么标记并阻塞消费者, 为了将来唤醒时知道唤醒哪个消费者 */
	while (ioq_empty(ioq)) {
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->consumer);
	    lock_release(&ioq->lock);
	}

	char byte = ioq->buf[ioq->tail];	/* 从缓冲队列中获取字符 */
	ioq->tail = next_pos(ioq->tail);

	if (ioq->producer != NULL) {  /* 如果生产者存在，唤醒生产者 */
		wakeup(&ioq->producer);
	}
	return byte;
}

/* 生产者从io队列中加入一个字符 */
void ioq_putchar(struct ioqueue* ioq, char byte) {
	ASSERT(intr_get_status() == INTR_OFF);
	/* 若缓冲区为满， 那么标记并阻塞生产者，为了将来唤醒时知道是哪个生产者 */
	while (ioq_full(ioq)) {
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->producer);
		lock_release(&ioq->lock);
	}

	ioq->buf[ioq->head] = byte;
	ioq->head = next_pos(ioq->head);
	if (ioq->consumer != NULL) {
		wakeup(&ioq->consumer);
	}
}







