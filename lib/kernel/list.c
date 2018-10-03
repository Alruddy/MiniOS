#include "list.h"
#include "interrupt.h"

/* 初始化双向链表 */
void list_init(struct list *plist) {
	plist->head.prev = NULL;
	plist->head.next = &plist->tail;
	plist->tail.prev = &plist->head;
	plist->tail.next = NULL;
}

/* 把链表元素elem插入到before之前 */
void list_insert_before(struct list_elem * before, struct list_elem * elem) {
	enum intr_status old_status = intr_disable();   /* 关中断 */

	before->prev->next = elem;
	elem->prev = before->prev;
	elem->next = before;
	before->prev = elem;

	intr_set_status(old_status);	/* 恢复状态 */
}

/* 添加链表元素到达队列首部 */
void list_push(struct list * plist, struct list_elem * elem) {
	list_insert_before(plist->head.next, elem);	/* 插入第一个元素之前 */
}

/* 追加链表元素到达队列尾部 */
void list_append(struct list * plist, struct list_elem * elem) {
	list_insert_before(&plist->tail, elem);		/* 插入队尾之前 */
}

/* 使元素elem脱离链表 */
void list_remove(struct list_elem * pelem) {
	enum intr_status old_status = intr_disable();

	pelem->prev->next = pelem->next;
	pelem->next->prev = pelem->prev;

	intr_set_status(old_status);
}

/* 弹出链表第一个元素 */
struct list_elem* list_pop(struct list * plist) {
	struct list_elem* pelem = plist->head.next;
	list_remove(pelem);
	return pelem;
}

/* 从链表中查找obj_elem,       成功返回true, 失败返回false */
bool elem_find(struct list * plist, struct list_elem * obj_elem) {
	struct list_elem *elem = plist->head.next;
	while(elem != &plist->tail) {
		if (elem == obj_elem) {
			return true;
		}
		elem = elem->next;
	}
	return false;
}

/* 遍历链表，查找满足条件的元素，返回指向元素的指针, 否则返回NULL */
struct list_elem* list_traversal(struct list * plist, function func, int arg) {
	struct list_elem *elem = plist->head.next;
	if (list_empty(plist)) {
		return NULL;
	}
	while(elem != &plist->tail) {
		if(func(plist, arg)) {
			return elem;
		}
		elem = elem->next;
	}
	return NULL;
}

/* 返回链表长度 */
uint32_t list_len(struct list * plist) {
	struct list_elem* elem = plist->head.next;
	uint32_t length = 0;
	while (elem != &plist->tail) {
		elem = elem->next;
		length++;
	}
	return length;
}

/* 判断链表为空 */
bool list_empty(struct list * plist) {
	return (plist->head.next == &plist->tail ? true : false);
}

