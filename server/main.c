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

#define MAX_EVENTS 256
#define PORT 9797

int set_nonblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag = flag | O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}

int main(int argc, char **argv) {
    int index;
    for(index = 0; index < 4; index++) {
        int pid = fork();
        if(pid == 0) {
            break;
        } else {
            // TODO: father code
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
                int n = 0;
                int nread;
                while((nread = read(fd, buf, 1024)) > 0) {
                    n += nread;
                }
                if(n == 0 || (nread == -1 && errno != EAGAIN)) {
                    printf("read error\n");
                    close(fd);
                } else {
                    ev.data.fd = fd;
                    ev.events = events[i].events | EPOLLOUT;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
                }
            } else if(events[i].events & EPOLLOUT) {
                const char *fmt = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s";
                const char *content = "hello world!";
                char buf[64];
                sprintf(buf, fmt, strlen(content), content);
                write(fd, buf, strlen(buf));
                close(fd);
            }
        }
    }
    free(events);
    close(epfd);
    return 0;
}
