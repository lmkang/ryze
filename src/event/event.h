#ifndef INCLUDE_EVENT_H_
#define INCLUDE_EVENT_H_

#include "list.h"

#define EV_LOCK(lock) pthread_mutex_lock(&lock->mutex)

#define EV_UNLOCK(lock) pthread_mutex_unlock(&lock->mutex)

#define EV_NOTIFY(lock) pthread_cond_signal(&lock->cond)

#define EV_WAIT(lock) pthread_cond_wait(&lock->cond, &lock->mutex)

#define EV_LOCK_INIT(lock) \
    pthread_mutex_init(&lock->mutex, NULL); \
    pthread_cond_init(&lock->cond, NULL);

#define EV_LOCK_DESTROY(lock) \
    pthread_mutex_destroy(&lock->mutex); \
    pthread_cond_destroy(&lock->cond);

namespace cattle {
namespace event {

struct ev_lock_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

struct ev_loop_t {
    int stop_flag;
    int epfd;
    int nfds;
    int wqfd;
    struct list_head *wq;
    struct ev_lock_t *wq_lock;
};

struct ev_loop_t *ev_loop_init();

void ev_loop_destroy(struct ev_loop_t *loop);

} // namespace event
} // namespace cattle

#endif // INCLUDE_EVENT_H_
