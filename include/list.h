#ifndef CATTLE_LIST_H_
#define CATTLE_LIST_H_

#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *) 0)->member)
#endif

#define container_of(ptr, type, member) ({ \
    (type *) ((char *) (ptr) - offsetof(type, member)); })

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)

#define list_for_each(head) \
    for(struct list_head *entry = (head)->next; entry != (head); entry = entry->next)

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

static inline void list_init(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *list, 
        struct list_head *prev, struct list_head *next) {
    next->prev = list;
    list->next = next;
    list->prev = prev;
    prev->next = list;
}

static inline void list_add(struct list_head *list, struct list_head *head) {
    __list_add(list, head, head->next);
}

static inline void list_add_tail(struct list_head *list, struct list_head *head) {
    __list_add(list, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

static inline int list_empty(const struct list_head *head) {
    return head->next == head;
}

static inline struct list_head *list_remove(struct list_head *head) {
    if(list_empty(head)) {
        return NULL;
    }
    struct list_head *entry = head->next;
    list_del(entry);
    return entry;
}

static inline struct list_head *list_remove_tail(struct list_head *head) {
    if(list_empty(head)) {
        return NULL;
    }
    struct list_head *entry = head->prev;
    list_del(entry);
    return entry;
}

#endif // CATTLE_LIST_H_
