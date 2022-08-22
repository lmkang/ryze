#ifndef RYZE_EIO_H
#define RYZE_EIO_H

#include "list.h"

#define EIO_REQ_GET(entry) LIST_ENTRY(entry, struct eio_req_t, entry)

#define EIO_REQ_EXEC(req) (req)->result = (req)->func((req)->arg)

#define EIO_REQ_FREE(req) (req)->destroy(req)

#define EIO_LOCK(mutex) pthread_mutex_lock(mutex)

#define EIO_UNLOCK(mutex) pthread_mutex_unlock(mutex)

#define EIO_WAIT(cond) pthread_cond_wait(cond)

#define EIO_SIGNAL(cond, mutex) pthread_cond_signal(cond, mutex)

enum eio_type {
    EIO_OPEN,
    EIO_STAT,
    EIO_READ,
    EIO_WRITE
};

struct eio_fs_t {
    ssize_t result;
    off_t offset;
    size_t size;
    void *ptr1;
    int fd;
    int flags;
    mode_t mode;
    int errno;
};

struct eio_req_t {
    struct list_head entry;
    enum eio_type type;
    void *arg;
    void *(*func)(void *arg);
    void *result;
    void (*destroy)(struct eio_req_t *req);
};

void etp_init(struct ev_loop_t *loop, int num);
void *etp_work(void *arg);

#endif // RYZE_EIO_H
