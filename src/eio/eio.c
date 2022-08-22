#include "eio.h"

void etp_init(struct ev_loop_t *loop, int num) {
    for(int i = 0; i < num; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, etp_work, loop);
        pthread_detach(tid);
    }
}

void *etp_work(void *arg) {
    struct ev_loop_t *loop = (struct ev_loop_t *) arg;
    struct list_head *entry;
    struct eio_req_t *req;
    uint64_t value = 1;
    size_t size = sizeof(uint64_t);
    while(1) {
        EIO_LOCK(loop->mutex[0]);
        if(loop->stop) {
            EIO_UNLOCK(loop->mutex[0]);
            return NULL;
        }
        while(LIST_EMPTY(loop->head[0])) {
            EIO_WAIT(loop->cond[0], loop->mutex[0]);
        }
        entry = loop->head[0]->next;
        LIST_DEL(entry);
        if(LIST_NOT_EMPTY(loop->head[0])) {
            EIO_SIGNAL(loop->cond[0]);
        }
        EIO_UNLOCK(loop->mutex[0]);
        req = EIO_REQ_GET(entry);
        EIO_REQ_EXEC(req);
        EIO_LOCK(loop->mutex[1]);
        LIST_ADD_TAIL(loop->head[1], entry);
        write(loop->efd, &value, size);
        EIO_UNLOCK(loop->mutex[1]);
    }
    return NULL;
}
