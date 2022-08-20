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

#define OFFSETOF(type, member) ((size_t) &((type *) 0)->member)

#define LIST_ENTRY(ptr, type, member) \
    (type *) ((char *) (ptr) - OFFSETOF(type, member));

#define LIST_ADD(tail, entry) \
    (tail)->next = (entry); \
    (tail) = (entry);

#define LIST_DEL(prev, entry) \
    (prev)->next = (entry)->next;
    

struct list_head {
    struct list_head *next;
};

struct ev_req_t {
    int finish;
    void *arg;
    void *(*func)(void *arg);
    void *result;
    void (*destroy)();
    struct list_head entry;
    atomic_int nsync;
};

struct ev_loop_t {
    int stop;
    int epfd;
    int efd;
    int nreq;
    struct ev_task_t *head;
    struct ev_task_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

char *read_file(const char *file_name);

int main(int argc, char **argv) {
    char *str = read_file("../js/file.txt");
    printf("file content: %s\n", str);
    free(str);
    return 0;
}

char *read_file(const char *file_name) {
    int fd = open(file_name, O_RDONLY);
    if(fd < 0) {
        printf("Can not open file: %s\n", file_name);
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


