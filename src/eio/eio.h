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
#define EIO_REQ_PTR_LEN 2

#define EIO_REQ_SETERR(req) \
    (req)->errnum = errno; \
    (req)->errbuf = malloc(sizeof(char) * EIO_REQ_ERRBUF_SIZE); \
    strerror_r(errno, (req)->errbuf, EIO_REQ_ERRBUF_SIZE)

#define EIO_REQ_ALLOC() \
    struct eio_req_t *req = malloc(sizeof(struct eio_req_t)); \
    for(int _i = 0; _i < EIO_REQ_PTR_LEN; _i++) { \
        req->ptr[_i] = NULL; \
    } \
    req->errnum = 0; \
    req->errbuf = NULL; \
    req->retbuf = NULL; \
    req->resolver = NULL; \
    req->work = NULL; \
    req->callback = NULL

#define EIO_REQ_FREE(req) \
    for(int _i = 0; _i < EIO_REQ_PTR_LEN; _i++) { \
        if((req)->ptr[_i] != NULL) { \
            free((req)->ptr[_i]); \
        } \
    } \
    if((req)->errbuf != NULL) { \
        free((req)->errbuf); \
    } \
    if((req)->retbuf != NULL) { \
        free((req)->retbuf); \
    } \
    free(req)

struct eio_req_t {
    struct list_head entry;
    int errnum;
    int retlen;
    ssize_t size;
    char *errbuf;
    void *retbuf;
    void *ptr[EIO_REQ_PTR_LEN];
    void *resolver;
    void (*work)(struct eio_req_t *req);
    void (*callback)(struct eio_req_t *req);
};

#endif // RYZE_EIO_H
