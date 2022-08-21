#ifndef RYZE_EIO_H
#define RYZE_EIO_H

#include "list.h"

#define EIO_REQ_GET(entry) LIST_ENTRY(entry, struct eio_req_t, entry)

#define EIO_REQ_EXEC(req) (req)->result = (req)->func((req)->arg)

#define EIO_REQ_DESTROY(req) (req)->destroy(req)

struct eio_req_t {
    struct list_head entry;
    void *arg;
    void *(*func)(void *arg);
    void *result;
    void (*destroy)(struct eio_req_t *req);
};

void etp_init(struct ev_loop_t *loop, int num);
void *etp_work(void *arg);

#endif // RYZE_EIO_H
