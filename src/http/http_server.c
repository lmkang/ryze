#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "http_parser.h"

struct http_buffer {
    char *data;
    size_t length;
    size_t offset;
    size_t capacity;
    struct list_head entry;
};

struct http_request {
    int fd;
    char *method;
    size_t method_len;
    char *path;
    size_t path_len;
    int minor_version;
    struct http_header *headers;
    int header_len;
    struct http_buffer *body_buffer;
    size_t content_length;
    int is_chunked;
    int is_keepalive;
    int keepalive_endtime;
    int keepalive_request;
    struct list_head sndbuf_list;
    struct list_head rcvbuf_list;
    struct list_head entry;
};

struct http_server {
    int fd;
    int sock_af;
    int sock_type;
    int sock_backlog;
    int so_reuseaddr;
    int so_reuseport;
    int so_keepalive;
    int tcp_nodelay;
    int keepalive_timeout;
    int keepalive_max;
    int port;
    char host[46];
    struct list_head request_list;
    void (*on_header_complete)(struct http_request *request);
};

int http_server_init(struct http_server *server) {
    int fd = socket(server->sock_af, server->sock_type, 0);
    if(fd < 0) {
        fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
        return -1;
    }
    do {
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &server->so_reuseaddr, sizeof(int))) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &server->so_reuseport, sizeof(int))) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &server->so_keepalive, sizeof(int))) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &server->tcp_nodelay, sizeof(int))) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        int flag;
        if((flag = fcntl(fd, F_GETFL)) < 0) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        flag = flag | O_NONBLOCK;
        if(fcntl(fd, F_SETFL, flag) < 0) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        server->fd = fd;
        LIST_INIT(&server->request_list);
        return 0;
    } while(0);
    close(fd);
    return -1;
}

void http_request_free(struct http_request *request) {
    if(close(request->fd)) {
        fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
    }
    struct http_buffer *buffer;
    struct list_head *entry;
    struct list_head *list = &request->sndbuf_list;
    while(!LIST_EMPTY(list)) {
        entry = list->next;
        LIST_DEL(entry);
        buffer = LIST_ENTRY(entry, struct http_buffer, entry);
        free(buffer);
    }
    list = &request->rcvbuf_list;
    while(!LIST_EMPTY(list)) {
        entry = list->next;
        LIST_DEL(entry);
        buffer = LIST_ENTRY(entry, struct http_buffer, entry);
        free(buffer);
    }
    free(request->headers);
    free(request);
}

void http_server_free(struct http_server *server) {
    if(close(server->fd)) {
        fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
    }
    struct list_head *request_list = &server->request_list;
    struct list_head *entry;
    struct http_request *request;
    while(!LIST_EMPTY(request_list)) {
        entry = request_list->next;
        LIST_DEL(entry);
        request = LIST_ENTRY(entry, struct http_request, entry);
        http_request_free(request);
    }
    free(server);
}

int http_server_listen(struct http_server *server) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = server->sock_af;
    addr.sin_port = htons(server->port);
    do {
        int ret = inet_pton(server->sock_type, server->host, &(addr.sin_addr.s_addr));
        if(ret == 0) {
            fprintf(stderr, "[inet_pton]: src is invalid network address\n");
            break;
        } else if(ret < 0) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        if(bind(server->fd, (struct sockaddr *) &addr, sizeof(addr))) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        if(listen(server->fd, server->sock_backlog)) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        return 0;
    } while(0);
    close(server->fd);
    return -1;
}

void http_server_on_accept(struct http_server *server) {
    
}

void http_server_on_keepalive(struct http_server *server) {
    
}
