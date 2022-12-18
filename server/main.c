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

struct ev_request_buf {
    char *buf;
    size_t offset;
    size_t length;
    struct ev_request_buf *next;
};

struct ev_request_data {
    int fd;
    struct ev_request_buf *head;
};

int set_nonblock(int fd);
char *read_file(const char *path);

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
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    struct epoll_event *events = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    while(1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if(nfds < 0) {
            printf("epoll wait error\n");
            break;
        }
        for(int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
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
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = connfd;
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
                char buf[1024];
                int n = read(fd, buf, 1024);
                if(n == -1 && errno != EAGAIN) {
                    printf("read error\n");
                    close(fd);
                } else {
                    if(n > 0) {
                        ev.data.fd = fd;
                        ev.events = EPOLLOUT | EPOLLET;
                        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
                    } else {
                        close(fd);
                    }
                }
            } else if(events[i].events & EPOLLOUT) {
                char *content = read_file("./test.txt");
                size_t len = strlen(content);
                const char *fmt = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s";
                char *buf = malloc(len + strlen(fmt) + 1);
                sprintf(buf, fmt, len, content);
                free(content);
                len = strlen(buf);
                int n = write(fd, buf, len);
                free(buf);
                if(n == len) {
                    close(fd);
                } else {
                    
                }
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
