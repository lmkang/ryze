#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "common.h"
#include "http_server.h"

int http_request_init(struct http_request *request) {
    request->header_len = 64;
    request->headers = malloc(request->header_len * sizeof(struct http_header));
    request->minor_version = 1;
    request->parse_status = 0;
    request->is_chunked = 0;
    request->is_keepalive = 1;
    request->content_length = -1;
    request->keepalive_request = 0;
    LIST_INIT(&request->sndbuf_list);
    LIST_INIT(&request->rcvbuf_list);
}

int http_request_parse(struct http_request *request) {
    struct list_head *rcvbuf_list = &request->rcvbuf_list;
    struct list_head *entry;
    struct linked_buffer *buffer;
    int ret = -1;
    while(!LIST_EMPTY(rcvbuf_list)) {
        entry = rcvbuf_list->next;
        buffer = LIST_ENTRY(entry, struct linked_buffer, entry);
        ret = http_parse_request_line(buffer->data, buffer->length, 
            &request->method, &request->method_len, &request->path, 
            &request->path_len, &request->minor_version);
        
    }
}

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
        if(listen(server->fd, server->backlog)) {
            fprintf(stderr, "[%d]: %s\n", errno, strerror(errno));
            break;
        }
        return 0;
    } while(0);
    close(server->fd);
    return -1;
}
