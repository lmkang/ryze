#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define MAX_EVENTS 128
#define PORT 9797

#define OFFSETOF(type, member) ((size_t) &((type *) 0)->member)

#define LIST_ENTRY(ptr, type, member) \
    (type *) ((char *) (ptr) - OFFSETOF(type, member))

#define LIST_INIT(head) \
    (head)->prev = (head); \
    (head)->next = (head)

#define LIST_ADD(head, entry) \
    (entry)->prev = (head); \
    (entry)->next = (head)->next; \
    (head)->next->prev = (entry); \
    (head)->next = (entry)

#define LIST_ADD_TAIL(head, entry) \
    (entry)->prev = (head)->prev; \
    (entry)->next = (head); \
    (head)->prev->next = (entry); \
    (head)->prev = (entry)

#define LIST_DEL(entry) \
    (entry)->prev->next = (entry)->next; \
    (entry)->next->prev = (entry)->prev; \
    (entry)->prev = NULL; \
    (entry)->next = NULL

#define LIST_EMPTY(head) ((head)->next == (head))

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

struct ev_rwbuf {
    struct list_head entry;
    ssize_t offset;
    ssize_t length;
    char *data;
};

struct ev_request {
    struct list_head list;
    int fd;
    void (*on_read)(struct ev_request *req);
    void (*on_write)(struct ev_request *req);
};

int set_nonblock(int fd);
char *read_file(const char *path);
void http_response_write(struct ev_request *req);
void http_on_read(struct ev_request *req);
void http_on_write(struct ev_request *req);

int main(int argc, char **argv) {
    int index;
    for(index = 0; index < 1; index++) {
        int pid = fork();
        if(pid == 0) {
            // child code
            break;
        } else if(pid > 0) {
            // TODO: father code
        } else {
            // TODO: error
        }
    }
    if(index >= 4) {
        return 0;
    }
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int sockopt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &sockopt, sizeof(sockopt));
    set_nonblock(listenfd);
    struct sockaddr_in local_addr;
    bzero(&local_addr, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(PORT);
    bind(listenfd, (struct sockaddr *) &local_addr, sizeof(local_addr));
    listen(listenfd, 128);
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    struct ev_request *ev_req = malloc(sizeof(struct ev_request));
    ev_req->fd = listenfd;
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = ev_req;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    struct epoll_event *events = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    while(1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if(nfds < 0) {
            printf("epoll wait error\n");
            break;
        }
        for(int i = 0; i < nfds; i++) {
            ev_req = events[i].data.ptr;
            int fd = ev_req->fd;
            if(events[i].events & EPOLLERR 
                || events[i].events & EPOLLHUP
                || (!events[i].events & EPOLLIN)) {
                close(fd);
                continue;
            }
            if(fd == listenfd) {
                int connfd;
                struct sockaddr_in addr;
                socklen_t addrlen;
                while((connfd = accept(listenfd, (struct sockaddr *) &addr, &addrlen)) > 0) {
                    set_nonblock(connfd);
                    struct ev_request *req = malloc(sizeof(struct ev_request));
                    LIST_INIT(&req->list);
                    req->fd = connfd;
                    req->on_read = http_on_read;
                    req->on_write = http_on_write;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.ptr = req;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
                }
                if(connfd == -1 
                    && errno != EAGAIN 
                    && errno != ECONNABORTED 
                    && errno != EPROTO
                    && errno != EINTR) {
                    printf("accept error\n");
                }
            } else if(events[i].events & EPOLLIN) {
                char tmp[1024];
                int n = read(fd, tmp, 1024);
                if(n == -1 && errno != EAGAIN) {
                    printf("read error\n");
                    close(fd);
                } else {
                    if(n > 0) {
                        http_response_write(ev_req);
                        ev.data.ptr = ev_req;
                        ev.events = EPOLLOUT | EPOLLET;
                        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
                    } else {
                        close(fd);
                    }
                }
            } else if(events[i].events & EPOLLOUT) {
                ev_req->on_write(ev_req);
            }
        }
    }
    free(events);
    close(epfd);
    return 0;
}

int set_nonblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag = flag | O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}

char *read_file(const char *path) {
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        printf("Error: Can not open file %s\n", path);
        return NULL;
    }
    struct stat st;
    fstat(fd, &st);
    off_t size = st.st_size;
    char *buf = malloc(size + 1);
    read(fd, buf, size);
    buf[size] = '\0';
    close(fd);
    return buf;
}

void http_response_write(struct ev_request *req) {
    char *content = read_file("./test.txt");
    size_t len = strlen(content);
    const char *fmt = "HTTP/1.1 200 OK\r\n\r\n\r\n%s";
    char *buf = malloc(len + strlen(fmt));
    sprintf(buf, fmt, content);
    free(content);
    len = strlen(buf);
    struct ev_rwbuf *rwbuf = malloc(sizeof(struct ev_rwbuf));
    rwbuf->offset = 0;
    rwbuf->length = len;
    rwbuf->data = buf;
    LIST_ADD_TAIL(&req->list, &rwbuf->entry);
}

void http_on_read(struct ev_request *req) {
    // TODO
}

void http_on_write(struct ev_request *req) {
    while(!LIST_EMPTY(&req->list)) {
        struct list_head *entry = req->list.next;
        struct ev_rwbuf *rwbuf = LIST_ENTRY(entry, struct ev_rwbuf, entry);
        int is_full = 0;
        while(rwbuf->offset < rwbuf->length - 1) {
            ssize_t n = write(req->fd, rwbuf->data + rwbuf->offset, rwbuf->length - rwbuf->offset);
            if(n > 0) {
                rwbuf->offset += n;
            } else {
                is_full = 1;
                break;
            }
        }
        if(!is_full) {
            LIST_DEL(entry);
            free(rwbuf->data);
            free(rwbuf);
        } else {
            break;
        }
    }
    if(LIST_EMPTY(&req->list)) {
        close(req->fd);
        free(req);
    }
}
