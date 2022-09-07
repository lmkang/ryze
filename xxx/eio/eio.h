#ifndef RYZE_EIO_H
#define RYZE_EIO_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include "list.h"

#define EIO_REQ_ERRBUF_SIZE 128
#define EIO_REQ_ARG_LEN 3
#define EIO_REQ_PTR_LEN 3

#define EIO_REQ_SETERR(req) \
    (req)->errnum = errno; \
    (req)->errbuf = malloc(sizeof(char) * EIO_REQ_ERRBUF_SIZE); \
    strerror_r(errno, (req)->errbuf, EIO_REQ_ERRBUF_SIZE)

#define EIO_REQ_ALLOC() \
    struct eio_req_t *req = (struct eio_req_t *) malloc(sizeof(struct eio_req_t)); \
    req->errnum = 0; \
    for(int _i = 0; _i < EIO_REQ_PTR_LEN; _i++) { \
        req->ptr[_i] = NULL; \
    } \
    req->errbuf = NULL

#define EIO_REQ_FREE(req) \
    for(int _i = 0; _i < EIO_REQ_PTR_LEN; _i++) { \
        if((req)->ptr[_i] != NULL) { \
            free((req)->ptr[_i]); \
        } \
    } \
    if((req)->errbuf != NULL) { \
        free((req)->errbuf); \
    } \
    free(req)

union eio_work_arg {
    int fd;
    int amode;
    int flag;
    int count;
    mode_t mode;
    uid_t owner;
    gid_t group;
    off_t len;
    size_t size;
};

union eio_work_ret {
    int fd;
    ssize_t size;
};

struct eio_req_t {
    struct list_head entry;
    union eio_work_arg args[EIO_REQ_ARG_LEN];
    union eio_work_ret ret;
    void *ptr[EIO_REQ_PTR_LEN];
    int errnum;
    char *errbuf;
    void *resolver;
    void (*work)(struct eio_req_t *req);
    void (*callback)(struct eio_req_t *req);
};

void eio_close(struct eio_req_t *req);
void eio_open(struct eio_req_t *req);
void eio_read(struct eio_req_t *req);

#endif // RYZE_EIO_H
