#ifndef RYZE_COMMON_H
#define RYZE_COMMON_H

#include "list.h"

struct linked_buffer {
    char *data;
    size_t length;
    size_t capacity;
    struct list_head entry;
};

#endif
