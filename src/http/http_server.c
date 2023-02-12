#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "http_parser.h"

struct http_buffer {
    struct list_head entry;
    char *data;
    size_t length;
    size_t offset;
};

struct http_keep_alive {
    struct list_head entry;
    int timeout;
    int request;
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
    struct http_keep_alive keep_alive;
};

struct http_server {
    int fd;
    struct list_head buffer_list;
    struct list_head keep_alive_list;
    void (*on_header_complete)(struct http_request *request);
};

void http_on_read(int fd) {
    
}

void http_on_write(int fd) {
    
}


