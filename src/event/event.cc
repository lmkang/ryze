#include "event.h"

struct ev_loop_t *ev_loop_init() {
    struct ev_loop_t *loop = (struct ev_loopt_t *) malloc(sizeof(struct ev_loopt_t));
    loop->stop_flag = 0;
    loop->wq = (struct list_head *) malloc(sizeof(struct list_head));
    loop->wq_lock = (struct ev_lock_t *) malloc(sizeof(struct ev_lock_t));
    list_init(loop->wq);
    EV_LOCK_INIT(loop->wq_lock);
    return loop;
}

void ev_loop_destroy(struct ev_loop_t *loop) {
    free(loop->wq);
    EV_LOCK_DESTROY(loop->wq_lock);
    free(loop->wq_lock);
}
