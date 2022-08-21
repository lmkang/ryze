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
    struct ev_req_t *req;
    uint64_t value = 1;
    size_t size = sizeof(uint64_t);
    while(1) {
        pthread_mutex_lock(loop->mutex[0]);
        while(LIST_EMPTY(loop->head[0])) {
            //pthread_cond_wait(loop->cond[0], loop->mutex[0]);
            pthread_mutex_unlock(loop->mutex[0]);
            return NULL;
        }
        entry = loop->head[0]->next;
        LIST_DEL(entry);
        if(LIST_NOT_EMPTY(loop->head[0])) {
            //pthread_cond_signal(loop->cond[0]);
        }
        pthread_mutex_unlock(loop->mutex[0]);
        req = EIO_REQ_GET(entry);
        EIO_REQ_EXEC(req);
        pthread_mutex_lock(loop->mutex[1]);
        LIST_ADD_TAIL(loop->head[1], entry);
        write(loop->efd, &value, size);
        pthread_mutex_unlock(loop->mutex[1]);
    }
    return NULL;
}
