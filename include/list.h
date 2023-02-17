#ifndef RYZE_LIST_H
#define RYZE_LIST_H

#define OFFSETOF(type, member) ((size_t) &((type *) 0)->member)

#define LIST_ENTRY(ptr, type, member) \
    (type *) ((char *) (ptr) - OFFSETOF(type, member))

#define LIST_INIT(head) \
    (head)->prev = (head); \
    (head)->next = (head)

#define LIST_ADD(head, entry) \
    (entry)->prev = (head); \
    (entry)->next = (head)->next; \
    (head)->next->prev = (entry); \
    (head)->next = (entry)

#define LIST_ADD_TAIL(head, entry) \
    (entry)->prev = (head)->prev; \
    (entry)->next = (head); \
    (head)->prev->next = (entry); \
    (head)->prev = (entry)

#define LIST_DEL(entry) \
    (entry)->prev->next = (entry)->next; \
    (entry)->next->prev = (entry)->prev; \
    (entry)->prev = NULL; \
    (entry)->next = NULL

#define LIST_EMPTY(head) ((head)->next == (head))

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

#endif
