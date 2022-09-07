#include "ev.h"

static void *ev_work(void *arg) {
    struct ev_loop *loop = arg;
    struct list_head *entry = NULL;
    struct eio_req_t *req = NULL;
    uint64_t value = 1;
    size_t size = sizeof(uint64_t);
    while(1) {
        pthread_mutex_lock(&loop->mutex[0]);
        while(LIST_EMPTY(&loop->list[0])) {
            loop->wait_threads += 1;
            pthread_cond_wait(&loop->cond[0], &loop->mutex[0]);
            loop->wait_threads -= 1;
        }
        entry = loop->list[0].next;
        LIST_DEL(entry);
        if(LIST_NOT_EMPTY(&loop->list[0]) && loop->wait_threads > 0) {
            pthread_cond_signal(&loop->cond[0]);
        }
        pthread_mutex_unlock(&loop->mutex[0]);
        req = LIST_ENTRY(entry, struct ev_req, entry);
        req->work(req);
        pthread_mutex_lock(&loop->mutex[1]);
        LIST_ADD_TAIL(&loop->list[1], entry);
        write(loop->efd, &value, size);
        pthread_mutex_unlock(&loop->mutex[1]);
    }
    return NULL;
}

struct ev_loop *ev_loop_init(int nthreads) {
    struct ev_loop *loop = malloc(sizeof(struct ev_loop));
    loop->epfd = epoll_create1(EPOLL_CLOEXEC);
    loop->efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = loop->efd;
    epoll_ctl(loop->epfd, EPOLL_CTL_ADD, loop->efd, &ev);
    for(int i = 0; i < 2; i++) {
        LIST_INIT(&loop->list[i]);
        pthread_mutex_init(&loop->mutex[i], NULL);
        pthread_cond_init(&loop->cond[i], NULL);
    }
    loop->nthreads = nthreads;
    loop->wait_threads = 0;
    for(int i = 0; i < nthreads; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, ev_work, loop);
        pthread_detach(tid);
    }
    return loop;
}

void ev_loop_destroy(struct ev_loop *loop) {
    close(loop->efd);
    close(loop->epfd);
    for(int i = 0; i < 2; i++) {
        pthread_mutex_destroy(&loop->mutex[i]);
        pthread_cond_destroy(&loop->cond[i]);
    }
    free(loop);
}

void ev_loop_run(struct ev_loop *loop, 
        void (*callback)(struct ev_loop *loop, int fd)) {
    uint64_t value;
    size_t size = sizeof(uint64_t);
    int nfds = -1;
    struct list_head *entry;
    struct eio_req *req;
    while(1) {
        nfds = epoll_wait(loop->epfd, loop->events, EV_MAX_EVENTS, -1);
        if(nfds < 0) {
            break;
        }
        for(int i = 0; i < nfds; i++) {
            callback(loop, loop->events[i].data.fd);
        }
    }
}
