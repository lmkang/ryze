#include "ev.h"

static void *ev_work(void *arg) {
    struct ev_loop_t *loop = arg;
    struct list_head *entry;
    struct eio_req_t *req;
    uint64_t value = 1;
    size_t size = sizeof(uint64_t);
    while(1) {
        EV_LOCK(loop->lock[0]);
        while(LIST_EMPTY(loop->head[0])) {
            if(++loop->wait_threads >= loop->nthreads) {
                write(loop->efd, &value, size);
            }
            EV_WAIT(loop->lock[0]);
            loop->wait_threads -= 1;
            if(loop->stop) {
                loop->alive_threads -= 1;
                EV_SIGNAL(loop->lock[0]);
                EV_UNLOCK(loop->lock[0]);
                return NULL;
            }
        }
        entry = loop->head[0]->next;
        LIST_DEL(entry);
        if(LIST_NOT_EMPTY(loop->head[0])) {
            EV_SIGNAL(loop->lock[0]);
        }
        EV_UNLOCK(loop->lock[0]);
        req = LIST_ENTRY(entry, struct eio_req_t, entry);
        req->work(req);
        EV_LOCK(loop->lock[1]);
        LIST_ADD_TAIL(loop->head[1], entry);
        write(loop->efd, &value, size);
        EV_UNLOCK(loop->lock[1]);
    }
    return NULL;
}

struct ev_loop_t *ev_loop_init() {
    struct ev_loop_t *loop = malloc(sizeof(struct ev_loop_t));
    loop->stop = 0;
    loop->epfd = epoll_create1(EPOLL_CLOEXEC);
    loop->events = malloc(sizeof(struct epoll_event) * MAX_NEVENTS);
    loop->efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = loop->efd;
    epoll_ctl(loop->epfd, EPOLL_CTL_ADD, loop->efd, &ev);
    for(int i = 0; i < 2; i++) {
        loop->head[i] = malloc(sizeof(struct list_head));
        loop->head[i]->prev = loop->head[i];
        loop->head[i]->next = loop->head[i];
        loop->lock[i] = malloc(sizeof(struct ev_lock_t));
        EV_LOCK_INIT(loop->lock[i]);
    }
    loop->nthreads = 4;
    loop->wait_threads = 0;
    loop->alive_threads = loop->nthreads;
    for(int i = 0; i < loop->nthreads; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, ev_work, loop);
        pthread_detach(tid);
    }
    return loop;
}

void ev_loop_free(struct ev_loop_t *loop) {
    while(1) {
        EV_LOCK(loop->lock[0]);
        if(loop->alive_threads <= 0) {
            EV_UNLOCK(loop->lock[0]);
            break;
        }
        EV_UNLOCK(loop->lock[0]);
    }
    close(loop->efd);
    close(loop->epfd);
    free(loop->events);
    for(int i = 0; i < 2; i++) {
        free(loop->head[i]);
        EV_LOCK_FREE(loop->lock[i]);
        free(loop->lock[i]);
    }
    free(loop);
}

void ev_loop_run(struct ev_loop_t *loop) {
    uint64_t value;
    size_t size = sizeof(uint64_t);
    int nfds = -1;
    struct list_head *entry;
    struct eio_req_t *req;
    while(1) {
        nfds = epoll_wait(loop->epfd, loop->events, MAX_NEVENTS, -1);
        if(nfds <= 0) {
            break;
        }
        for(int i = 0; i < nfds; i++) {
            if(loop->events[i].data.fd == loop->efd 
                    && loop->events[i].events & EPOLLIN) {
                read(loop->events[i].data.fd, &value, size);
                EV_LOCK(loop->lock[1]);
                while(LIST_NOT_EMPTY(loop->head[1])) {
                    entry = loop->head[1]->next;
                    LIST_DEL(entry);
                    req = LIST_ENTRY(entry, struct eio_req_t, entry);
                    req->callback(req);
                    EIO_REQ_FREE(req);
                }
                EV_UNLOCK(loop->lock[1]);
            }
        }
    }
}
