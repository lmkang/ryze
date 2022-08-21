#include <stdlib.h>
#include "ev.h"

struct ev_loop_t *ev_loop_init() {
    struct ev_loop_t *loop = (struct ev_loop_t *) malloc(sizeof(struct ev_loop_t));
    loop->stop = 0;
    loop->epfd = epoll_create1(EPOLL_CLOEXEC);
    loop->nevs = 1024;
    loop->events = (struct epoll_event *) malloc(sizeof(struct epoll_event) * MAX_NEVENTS);
    loop->efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = loop->efd;
    epoll_ctl(loop->epfd, EPOLL_CTL_ADD, loop->efd, &ev);
    for(int i = 0; i < 2; i++) {
        loop->head[i] = (struct list_head *) malloc(sizeof(struct list_head));
        loop->head[i]->prev = loop->head[i];
        loop->head[i]->next = loop->head[i];
        loop->mutex[i] = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(loop->mutex[i], NULL);
        loop->cond[i] = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
        pthread_cond_init(loop->cond[i], NULL);
    }
    return loop;
}

void ev_loop_destroy(struct ev_loop_t *loop) {
    close(loop->efd);
    close(loop->epfd);
    free(loop->events);
    for(int i = 0; i < 2; i++) {
        free(loop->head[i]);
        pthread_mutex_destroy(loop->mutex[i]);
        pthread_cond_destroy(loop->cond[i]);
        free(loop->mutex[i]);
        free(loop->cond[i]);
    }
    free(loop);
}

void ev_loop_run(struct ev_loop_t *loop) {
    uint64_t value;
    size_t size = sizeof(uint64_t);
    int nfds = -1;
    while(1) {
        nfds = epoll_wait(loop->epfd, loop->events, MAX_NEVENTS, -1);
        if(nfds <= 0) {
            break;
        }
        for(int i = 0; i < nfds; i++) {
            if(loop->events[i].data.fd == loop->efd 
                    && loop->events[i].events & EPOLLIN) {
                read(loop->events[i].data.fd, &value, size);
                // TODO: process eventfd
            }
        }
    }
}
