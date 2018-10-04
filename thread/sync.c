#include "sync.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"

/* 初始化信号量 */
void sema_init(struct semaphore* psema, uint8_t value) {
	psema->value = value;			/* 信号量赋初值 */
	list_init(&psema->waiters);		/* 初始化等待队列 */
}

/* 初始化锁 */
void lock_init(struct lock* plock) {
	plock->holder = NULL;			/* 锁持有这 */
	plock->holder_repeat_nr = 0;	
	sema_init(&plock->semaphore, 1);/* 信号量初值为1 */
}

/* 信号量down操作       P */
void sema_down(struct semaphore* psema) {
	/* 关中断保证原子性 */
	enum intr_status old_status = intr_disable();
	while (psema->value == 0) {
		/* 当前线程不再队列中 */
		ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
		if (elem_find(&psema->waiters, &running_thread()->general_tag)) {
			PANIC("sema_down: thread blocked has been in waiters_list!!!\n");
		}
		list_append(&psema->waiters, &running_thread()->general_tag);
		thread_block(TASK_BLOCKED);
	}
	psema->value--;	/* P */
	ASSERT(psema->value == 0);
	intr_set_status(old_status);
}

/* 信号量up操作 V */
void sema_up(struct semaphore* psema) {
	/* 关中断保证原子性 */
	enum intr_status old_status = intr_disable();
	ASSERT(psema->value == 0);
	if (!list_empty(&psema->waiters)) { /* 有等待的线程 */
		struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, \
			list_pop(&psema->waiters));
		thread_unblock(thread_blocked);
	}
	psema->value++;
	ASSERT(psema->value == 1);
	intr_set_status(old_status);
}

/* 获取锁 */
void lock_acquire(struct lock* plock) {
	if (plock->holder != running_thread()) {
		sema_down(&plock->semaphore);	/* P */
		plock->holder = running_thread();
		ASSERT(plock->holder_repeat_nr == 0);
		plock->holder_repeat_nr = 1;
	} else {
		plock->holder_repeat_nr++;
	}
}

/* 释放锁 */
void lock_release(struct lock* plock) {
	ASSERT(plock->holder == running_thread());
	if (plock->holder_repeat_nr > 1) {
		plock->holder_repeat_nr--;
		return;
	}
	ASSERT(plock->holder_repeat_nr == 1);
	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_up(&plock->semaphore);			/* V */
}



