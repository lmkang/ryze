#ifndef RYZE_EV_H
#define RYZE_EV_H

#include <sys/epoll.h>
#include <pthread.h>
#include "list.h"

#define MAX_NEVENTS 1024

#define EV_LOCK(mutex) pthread_mutex_lock(mutex)

#define EV_UNLOCK(mutex) pthread_mutex_unlock(mutex)

#define EV_WAIT(cond) pthread_cond_wait(cond)

#define EV_SIGNAL(cond, mutex) pthread_cond_signal(cond, mutex)

struct ev_loop_t {
    int stop;
    int epfd;
    int efd;
    struct epoll_event *events;
    struct list_head *head[2];
    pthread_mutex_t *mutex[2];
    pthread_cond_t *cond[2];
};

struct ev_loop_t *ev_loop_init();
void ev_loop_destroy(struct ev_loop_t *loop);
void ev_loop_run(struct ev_loop_t *loop);

#endif // RYZE_EV_H
