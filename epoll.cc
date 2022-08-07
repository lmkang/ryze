#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "list.h"

#define MALLOC(type) (type *) malloc(sizeof(type))
#define NMALLOC(type, n) (type *) malloc(sizeof(type) * (n))
#define MAX_EVENTS 16

struct task_info {
    struct list_head entry;
    int thread_id;
    void *(*callback)(void *arg);
    void *arg;
};

struct thread_param {
    int fd;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    struct list_head *head;
};

void *thread_func(void *arg);
void *task_callback(void *arg);
void process_task(int fd, pthread_mutex_t *mutex, 
    pthread_cond_t *cond, struct list_head *head);
char *read_file(const char *file_name);

int main(int argc, char **argv) {
    int evfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = evfd;
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    epoll_ctl(epfd, EPOLL_CTL_ADD, evfd, &ev);
    
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    
    struct list_head *head = MALLOC(struct list_head);
    list_init(head);
    struct thread_param *param = MALLOC(struct thread_param);
    param->fd = evfd;
    param->mutex = &mutex;
    param->cond = &cond;
    param->head = head;
    for(int i = 0; i < 4; i++) {
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, thread_func, param);
        pthread_detach(thread_id);
    }
    
    struct epoll_event ep_events[MAX_EVENTS];
    int nfds = -1;
    while(1) {
        nfds = epoll_wait(epfd, ep_events, MAX_EVENTS, 3);
        printf("nfds: %d\n", nfds);
        if(nfds == 0) {
            break;
        }
        for(int i = 0; i < nfds; i++) {
            if(ep_events[i].events & EPOLLIN) {
                process_task(evfd, &mutex, &cond, head);
            }
        }
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    free(head);
    epoll_ctl(epfd, EPOLL_CTL_DEL, evfd, &ev);
    close(evfd);
    close(epfd);
    return 0;
}

void *thread_func(void *arg) {
    struct thread_param *param = (struct thread_param *) arg;
    int id = syscall(SYS_gettid);
    int count = 3;
    uint64_t value = 123;
    while(count) {
        pthread_mutex_lock(param->mutex);
        struct task_info *task = MALLOC(struct task_info);
        task->thread_id = id;
        task->callback = task_callback;
        task->arg = (void *) "./js/file.txt";
        list_add_tail(&task->entry, param->head);
        pthread_mutex_unlock(param->mutex);
        pthread_cond_signal(param->cond);
        write(param->fd, (void *) &value, sizeof(uint64_t));
        count--;
    }
    return NULL;
}

void *task_callback(void *arg) {
    return read_file((const char *) arg);
}

void process_task(int fd, pthread_mutex_t *mutex, 
        pthread_cond_t *cond, struct list_head *head) {
    pthread_mutex_lock(mutex);
    struct task_info *task;
    char *data;
    struct list_head *p = head->next;
    struct list_head *entry;
    while(p != head) {
        entry = p;
        p = p->next;
        list_del(entry);
        task = list_entry(entry, struct task_info, entry);
        data = (char *) task->callback(task->arg);
        printf("%d. file_name: %s\n", task->thread_id, (const char *) task->arg);
        printf("%d. file_content:  %s\n", task->thread_id, data);
        free(data);
        free(task);
    }
    pthread_mutex_unlock(mutex);
    pthread_cond_signal(cond);
    uint64_t result;
    read(fd, &result, sizeof(uint64_t));
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






























































































































































































































