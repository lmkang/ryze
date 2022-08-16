#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

struct ev_task_t {
    struct ev_task_t *next;
    int epfd;
    int fd;
    int finish;
    atomic_int count;
    void *arg;
    void *(*func)(void *arg);
    void *result;
    void (*destroy)(struct ev_task_t *task);
};

char *read_file(const char *file_name);

void *thread_func(void *arg) {
    struct ev_task_t *p = (struct ev_task_t *) arg;
    int evfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = evfd;
    epoll_ctl(p->epfd, EPOLL_CTL_ADD, evfd, &ev);
    p = p->next;
    while(1) {
        if(p == NULL) {
            break;
        }
        if(!atomic_fetch_add_explicit(&p->count, 1, memory_order_relaxed)) {
            p->result = p->func(p->arg);
            p->fd = evfd;
            uint64_t v = 1;
            write(evfd, &v, sizeof(uint64_t));
        }
        p = p->next;
    }
    close(evfd);
    return NULL;
}

void *task_func(void *arg) {
    return read_file((const char *) arg);
}

void task_destroy(struct ev_task_t *task) {
    free(task->result);
    free(task);
}

struct ev_task_t *task_alloc(int epfd, void *arg) {
    struct ev_task_t *task = (struct ev_task_t *) malloc(sizeof(struct ev_task_t));
    task->next = NULL;
    task->epfd = epfd;
    task->fd = -1;
    task->finish = 0;
    task->count = 0;
    task->arg = arg;
    task->func = task_func;
    task->destroy = task_destroy;
    return task;
}

void task_add(struct ev_task_t *tail, struct ev_task_t *task) {
    tail->next = task;
    tail = task;
}

void process_task(struct ev_task_t *head, int fd, uint64_t total) {
    struct ev_task_t *prev = head;
    struct ev_task_t *p = head->next;
    struct ev_task_t *tmp;
    uint64_t count = 0;
    while(p) {
        if(p->finish && (prev->finish || prev == head)) {
            tmp = p;
            prev->next = p->next;
            p = p->next;
            tmp->destroy(tmp);
            continue;
        }
        if(p->fd == fd && count < total) {
            p->result = p->func(p->arg);
            p->finish = 1;
            count++;
            printf("%s\n", (char *) p->result);
        }
        prev = p;
        p = p->next;
    }
}

int main(int argc, char **argv) {
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    struct ev_task_t *head = task_alloc(epfd, NULL);
    struct ev_task_t *tail = head;
    struct epoll_event events[128];
    int nfds = -1;
    
    for(int i = 0; i < 9; i++) {
        struct ev_task_t *task = task_alloc(epfd, (void *) "../js/file.txt");
        task_add(tail, task);
        tail = task;
    }
    for(int i = 0; i < 4; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, thread_func, head);
        pthread_detach(tid);
    }
    
    while(1) {
        nfds = epoll_wait(epfd, events, 128, 9000);
        if(nfds == 0) {
            printf("epoll break\n");
            break;
        }
        for(int i = 0; i < nfds; i++) {
            if(events[i].events & EPOLLIN) {
                uint64_t result;
                read(events[i].data.fd, &result, sizeof(uint64_t));
                process_task(head, events[i].data.fd, result);
            }
        }
    }
    close(epfd);
    return 0;
}

char *read_file(const char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if(file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    char *chars = (char *) malloc(sizeof(char) * (size + 1));
    chars[size] = '\0';
    for(size_t i = 0; i < size; ) {
        i += fread(&chars[i], 1, size - i, file);
        if(ferror(file)) {
            fclose(file);
            free(chars);
            return NULL;
        }
    }
    fclose(file);
    return chars;
}
