#ifndef __LIST_H_
#define __LIST_H_

#include <stdio.h>
#include <assert.h>
struct list_head {
    struct list_head* __next, *__prev;
};

static inline void list_head_init(struct list_head* head) {
    head->__next = head;
    head->__prev = head;
}

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->__next = (ptr); (ptr)->__prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __list_add(struct list_head* nnew,
                                  struct list_head* prev,
                                  struct list_head* next) {
    next->__prev = nnew;
    nnew->__next = next;
    nnew->__prev = prev;
    prev->__next = nnew;
}

/**
 * list_add - add a new entry
 * @nnew: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline__ void list_add(struct list_head* nnew, struct list_head* head) {
    __list_add(nnew, head, head->__next);
}

/**
 * list_add_tail - add a new entry
 * @nnew: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline__ void list_add_tail(struct list_head* nnew, struct list_head* head) {
    __list_add(nnew, head->__prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __list_del(struct list_head* prev,
                                  struct list_head* next) {
    next->__prev = prev;
    prev->__next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
#define list_del(e) internal_list_del((e),(__FILE__),(__LINE__))
static __inline__ void internal_list_del(struct list_head* entry, const char* f, int l) {
    __list_del(entry->__prev, entry->__next);
    entry->__next = entry->__prev = entry;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
static __inline__ void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}
 */

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static __inline__ int list_empty(struct list_head* head) {
    assert(head->__next);
    return head->__next == head;
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
static __inline__ void list_splice(struct list_head *list, struct list_head *head)
{
    struct list_head *first = list->next;

    if (first != list) {
        struct list_head *last = list->prev;
        struct list_head *at = head->next;

        first->prev = head;
        head->next = first;

        last->next = at;
        at->prev = last;
    }
}
 */

static __inline__ struct list_head* list_first(struct list_head* head) {
    return head->__next;
}

static __inline__ struct list_head* list_last(struct list_head* head) {
    return head->__prev;
}
/**
 * list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_for_each    -   iterate over a list
 * @pos:    the &struct list_head to use as a loop counter.
 * @head:   the head for your list.
 */
#define list_for_each(pos, head) \
    for (pos = (head)->__next; pos != (head); \
            pos = pos->__next)

/**
 * list_for_each_safe   -   iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop counter.
 * @n:      another &struct list_head to use as temporary storage
 * @head:   the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->__next, n = pos->__next; pos != (head); \
        pos = n, n = pos->__next)

#define list_for_each_reverse_safe(pos, n, head) \
    for (pos = (head)->__prev, n = pos->__prev; pos != (head); \
        pos = n, n = pos->__prev)

#define list_for_each_clear(pos, head) \
    for (pos = (head)->__next; !list_empty(head); \
            list_del(pos), pos = (head)->__next)

#endif
