#ifndef RYZE_EV_H
#define RYZE_EV_H

#include <stdlib.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "list.h"
#include "eio.h"

#define MAX_NEVENTS 1024

#define EV_LOCK_INIT(lock) \
    pthread_mutex_init(&(lock)->mutex, NULL); \
    pthread_cond_init(&(lock)->cond, NULL)

#define EV_LOCK_FREE(lock) \
    pthread_mutex_destroy(&(lock)->mutex); \
    pthread_cond_destroy(&(lock)->cond)

#define EV_LOCK(lock) pthread_mutex_lock(&(lock)->mutex)

#define EV_UNLOCK(lock) pthread_mutex_unlock(&(lock)->mutex)

#define EV_WAIT(lock) pthread_cond_wait(&(lock)->cond, &(lock)->mutex)

#define EV_SIGNAL(lock) pthread_cond_signal(&(lock)->cond)

struct ev_lock_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

struct ev_loop_t {
    int stop;
    int epfd;
    int efd;
    int active_threads;
    struct epoll_event *events;
    struct list_head *head[2];
    struct ev_lock_t *lock[2];
};

struct ev_loop_t *ev_loop_init();
void ev_loop_free(struct ev_loop_t *loop);
void ev_loop_run(struct ev_loop_t *loop);
void ev_loop_stop(struct ev_loop_t *loop);
void ev_req_add(struct ev_loop_t *loop, struct eio_req_t *req);

#endif // RYZE_EV_H
