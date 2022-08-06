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

#define MAX_EVENTS 16

namespace cattle {
namespace list {

struct item {
    struct item *prev;
    struct item *next;
    void *data;
};

struct list {
    struct item *head;
    struct item *tail;
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
};

struct list *list_init() {
    struct list *li = (struct list *) malloc(sizeof(struct list));
    li->head = NULL;
    li->tail = NULL;
    li->cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    li->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_cond_init(li->cond, NULL);
    pthread_mutex_init(li->mutex, NULL);
    return li;
}

void list_free(struct list *li) {
    pthread_cond_destroy(li->cond);
    pthread_mutex_destroy(li->mutex);
    free(li->cond);
    free(li->mutex);
    free(li);
}

void list_add(struct list *li, void *data) {
    struct item *it = (struct item *) malloc(sizeof(struct item));
    it->prev = NULL;
    it->next = NULL;
    it->data = data;
    pthread_mutex_lock(li->mutex);
    if(li->head == NULL) {
        li->head = it;
        li->tail = it;
    } else {
        li->tail->next = it;
        it->prev = li->tail;
        li->tail = it;
    }
    pthread_mutex_unlock(li->mutex);
    pthread_cond_signal(li->cond);
}

struct item *list_remove(struct list *li) {
    struct item *it = NULL;
    pthread_mutex_lock(li->mutex);
    if(li->tail != NULL) {
        it = li->tail;
        li->tail = li->tail->prev;
        li->tail->next = NULL;
        it->prev = NULL;
    }
    pthread_mutex_unlock(li->mutex);
    pthread_cond_signal(li->cond);
    return it;
}

} // namespace list
} // namespace cattle

using namespace cattle;

struct task_info {
    void *(*callback)(void *arg);
    void *arg;
};

struct thread_param {
    int fd;
    struct list::list *task_list;
};

void *thread_func(void *arg);
void *task_callback(void *arg);
void process_task(int fd, struct list::list *task_list);
void setnonblock(int fd);
char *read_file(const char *file_name);

struct epoll_event ep_events[MAX_EVENTS];
int epfd = -1;
int evfd = -1;

int main(int argc, char **argv) {
    struct list::list *task_list = list::list_init();
    evfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = evfd;
    epfd = epoll_create1(EPOLL_CLOEXEC);
    epoll_ctl(epfd, EPOLL_CTL_ADD, evfd, &ev);
    
    struct thread_param *param = (struct thread_param *) malloc(sizeof(struct thread_param));
    param->fd = evfd;
    param->task_list = task_list;
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, thread_func, param);
    pthread_detach(thread_id);
    
    int nfds = -1;
    uint64_t ev_result;
    for(;;) {
        nfds = epoll_wait(epfd, ep_events, MAX_EVENTS, -1);
        printf("nfds: %d\n", nfds);
        for(int i = 0; i < nfds; i++) {
            if(ep_events[i].events & EPOLLIN) {
                process_task(ep_events[i].data.fd, task_list);
            }
        }
    }
    list::list_free(task_list);
    close(evfd);
    close(epfd);
    return 0;
}

void *thread_func(void *arg) {
    sleep(3);
    struct thread_param *param = (struct thread_param *) arg;
    struct task_info *task = (struct task_info *) malloc(sizeof(struct task_info));
    task->callback = task_callback;
    task->arg = (void *) "./fs/file.txt";
    list::list_add(param->task_list, task);
    uint64_t value = 123;
    write(param->fd, (void *) value, sizeof(uint64_t));
    return NULL;
}

void *task_callback(void *arg) {
    return read_file((const char *) arg);
}

void process_task(int fd, struct list::list *task_list) {
    struct list::item *it = list::list_remove(task_list);
    struct task_info *task = (struct task_info *) it->data;
    char *data = (char *) task->callback(task->arg);
    printf("task content: %s\n", data);
    free(data);
    free(task);
    free(it);
    uint64_t result;
    read(fd, &result, sizeof(uint64_t));
}

void setnonblock(int fd) {
    int opts = fcntl(fd, F_GETFL);
    if(opts < 0) {
        perror("fcntl(F_GETFL)\n");
    }
    opts |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)\n");
    }
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






























































































































































































































