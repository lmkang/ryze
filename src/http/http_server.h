#ifndef RYZE_HTTP_SERVER_H
#define RYZE_HTTP_SERVER_H

struct http_server {
    int fd;
    int sock_af;
    int sock_type;
    int backlog;
    int so_reuseaddr;
    int so_reuseport;
    int so_keepalive;
    int tcp_nodelay;
    int keepalive_timeout;
    int keepalive_max;
    int min_header_size;
    int max_header_size;
    int port;
    char host[46];
    struct list_head request_list;
    void (*on_header_complete)(struct http_request *request);
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
    int parse_status;
    size_t body_offset;
    struct http_server *server;
    int is_chunked;
    int is_keepalive;
    int64_t content_length;
    int64_t keepalive_endtime;
    int keepalive_request;
    struct list_head sndbuf_list;
    struct list_head rcvbuf_list;
    struct list_head entry;
};

#endif
