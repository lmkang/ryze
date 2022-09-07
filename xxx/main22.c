#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include "list.h"

struct ev_loop_t {
    int stop;
    int epfd;
    int efd;
    int nevs;
    struct epoll_event *events;
    struct list_head *head[2];
    pthread_mutex_t *mutex[2];
    pthread_cond_t *cond[2];
};

struct ev_req_t {
    struct list_head entry;
    void *arg;
    void *(*func)(void *arg);
    void *result;
    void (*destroy)();
};

void process_work(struct ev_loop_t *loop);
struct ev_loop_t *ev_loop_init();
void ev_loop_destroy(struct ev_loop_t *loop);
void *ev_fs_read(void *pathname);
void ev_fs_destroy(struct ev_req_t *req);
void *etp_work(void *arg);

int main(int argc, char **argv) {
    struct ev_loop_t *loop = ev_loop_init();
    for(int i = 0; i < 9; i++) {
        struct ev_req_t *req = (struct ev_req_t *) malloc(sizeof(struct ev_req_t));
        req->arg = (void *) "../js/file.txt";
        req->func = ev_fs_read;
        req->destroy = ev_fs_destroy;
        LIST_ADD_TAIL(loop->head[0], &req->entry);
    }
    for(int i = 0; i < 4; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, etp_work, loop);
        pthread_detach(tid);
    }
    int nfds = -1;
    uint64_t result;
    while(1) {
        nfds = epoll_wait(loop->epfd, loop->events, loop->nevs, 3000);
        if(nfds == 0) {
            printf("epoll break\n");
            break;
        }
        for(int i = 0; i < nfds; i++) {
            if(loop->events[i].data.fd == loop->efd 
                    && loop->events[i].events & EPOLLIN) {
                read(loop->events[i].data.fd, &result, sizeof(uint64_t));
                process_work(loop);
            }
        }
    }
    ev_loop_destroy(loop);
    return 0;
}

void process_work(struct ev_loop_t *loop) {
    struct list_head *entry;
    struct ev_req_t *req;
    pthread_mutex_lock(loop->mutex[1]);
    while(1) {
        if(LIST_EMPTY(loop->head[1])) {
            pthread_mutex_unlock(loop->mutex[1]);
            break;
        }
        entry = loop->head[1]->next;
        LIST_DEL(entry);
        req = LIST_ENTRY(entry, struct ev_req_t, entry);
        req->destroy(req);
        free(req);
    }
}

struct ev_loop_t *ev_loop_init() {
    struct ev_loop_t *loop = (struct ev_loop_t *) malloc(sizeof(struct ev_loop_t));
    loop->stop = 0;
    loop->epfd = epoll_create1(EPOLL_CLOEXEC);
    loop->nevs = 1024;
    loop->events = (struct epoll_event *) malloc(sizeof(struct epoll_event) * loop->nevs);
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

unsigned seed = 43299132;

void *ev_fs_read(void *pathname) {
    srand(seed);
    sleep(rand() % 3 + 1);
    int fd = open(pathname, O_RDONLY);
    if(fd < 0) {
        printf("Can not open file: %s\n", pathname);
        return NULL;
    }
    struct stat st;
    fstat(fd, &st);
    off_t size = st.st_size;
    char *buf = (char *) malloc(sizeof(char) * (size + 1));
    buf[size] = '\0';
    read(fd, buf, size);
    close(fd);
    return buf;
}

void ev_fs_destroy(struct ev_req_t *req) {
    free(req->result);
}

void *etp_work(void *arg) {
    struct ev_loop_t *loop = (struct ev_loop_t *) arg;
    int tid = syscall(SYS_gettid);
    struct list_head *entry;
    struct ev_req_t *req;
    uint64_t value = 1;
    size_t size = sizeof(uint64_t);
    while(1) {
        pthread_mutex_lock(loop->mutex[0]);
        while(LIST_EMPTY(loop->head[0])) {
            //pthread_cond_wait(loop->cond[0], loop->mutex[0]);
            pthread_mutex_unlock(loop->mutex[0]);
            printf("thread %d break\n", tid);
            return NULL;
        }
        entry = loop->head[0]->next;
        LIST_DEL(entry);
        if(LIST_NOT_EMPTY(loop->head[0])) {
            //pthread_cond_signal(loop->cond[0]);
        }
        pthread_mutex_unlock(loop->mutex[0]);
        req = LIST_ENTRY(entry, struct ev_req_t, entry);
        req->result = req->func(req->arg);
        printf("thread %d: %s\n", tid, (char *) req->result);
        pthread_mutex_lock(loop->mutex[1]);
        LIST_ADD_TAIL(loop->head[1], entry);
        write(loop->efd, &value, size);
        pthread_mutex_unlock(loop->mutex[1]);
    }
    return NULL;
}
