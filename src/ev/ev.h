#ifndef RYZE_EV_H
#define RYZE_EV_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "list.h"

#define EV_REQ_ERRBUF_SIZE 128

#define EV_REQ_ARG_LEN 3

#define EV_REQ_PTR_LEN 3

#define EV_MAX_EVENTS 1024

#define EV_REQ_ADD(loop, req) \
    pthread_mutex_lock(&(loop)->mutex[0]); \
    LIST_ADD_TAIL((loop)->list[0], &(req)->entry); \
    if((loop)->wait_threads > 0) { \
        pthread_cond_signal(&(loop)->cond[0]); \
    } \
    pthread_mutex_unlock(&(loop)->mutex[0])

union ev_req_arg {
    int fd;
    int amode;
    int flag;
    int count;
    mode_t mode;
    uid_t owner;
    gid_t group;
    off_t len;
    size_t size;
};

union ev_req_ret {
    int fd;
    ssize_t size;
};

struct ev_req {
    struct list_head entry;
    union ev_req_arg args[EV_REQ_ARG_LEN];
    union ev_req_ret ret;
    void *ptr[EV_REQ_PTR_LEN];
    int errnum;
    char *errbuf;
    void *resolver;
    void (*work)(struct ev_req *req);
    void (*callback)(struct ev_req *req);
};

struct ev_loop {
    int epfd;
    int efd;
    int nthreads;
    int wait_threads;
    struct list_head list[2];
    pthread_mutex_t mutex[2];
    pthread_cond_t cond[2];
    struct epoll_event events[EV_MAX_EVENTS];
};

#endif
