#ifndef RYZE_EIO_H
#define RYZE_EIO_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include "list.h"

#define EIO_REQ_START(req) \
    union eio_fs_ret *ret = malloc(sizeof(union eio_fs_ret)); \
    union eio_fs_arg *args = (req)->args

#define EIO_REQ_END(req) (req)->ret = ret

#define EIO_REQ_FREE(req) \
    if((req)->free != NULL) { \
        (req)->free(req); \
    } \
    if((req)->args != NULL) { \
        free((req)->args); \
    } \
    if((req)->ret != NULL) { \
        free((req)->ret); \
    } \
    free(req)

enum eio_ret_type {
    EIO_RET_INT,
    EIO_RET_SSIZE_T,
    EIO_RET_CHAR_PTR,
    EIO_RET_DIR_PTR,
    EIO_RET_DIRENT_PTR
};

union eio_fs_arg {
    int fd;
    int mode1;
    int flag;
    int count;
    size_t size;
    mode_t mode;
    off_t len;
    uid_t owner;
    gid_t group;
    void *path;
    void *path1;
    void *path2;
    void *buf;
    void *ptr;
};

union eio_fs_ret {
    int err;
    ssize_t size;
    void *ptr;
};

struct eio_req_t {
    struct list_head entry;
    void *args;
    int argc;
    void *ret;
    enum eio_ret_type ret_type;
    void *resolver;
    void (*work)(struct eio_req_t *req);
    void (*free)(struct eio_req_t *req);
};

struct eio_req_t *eio_req_alloc(int argc, int ret);
void eio_access(struct eio_req_t *req);
void eio_chmod(struct eio_req_t *req);
void eio_chown(struct eio_req_t *req);
void eio_close(struct eio_req_t *req);
void eio_fchmod(struct eio_req_t *req);
void eio_fchown(struct eio_req_t *req);
void eio_fdatasync(struct eio_req_t *req);
void eio_fstat(struct eio_req_t *req);
void eio_fsync(struct eio_req_t *req);
void eio_ftruncate(struct eio_req_t *req);
void eio_futimes(struct eio_req_t *req);
void eio_lchown(struct eio_req_t *req);
void eio_lutimes(struct eio_req_t *req);
void eio_link(struct eio_req_t *req);
void eio_lstat(struct eio_req_t *req);
void eio_mkdir(struct eio_req_t *req);
void eio_mkdtemp(struct eio_req_t *req);
void eio_open(struct eio_req_t *req);
void eio_opendir(struct eio_req_t *req);
void eio_read(struct eio_req_t *req);
void eio_readdir(struct eio_req_t *req);
void eio_readlink(struct eio_req_t *req);
void eio_readv(struct eio_req_t *req);
void eio_realpath(struct eio_req_t *req);
void eio_rename(struct eio_req_t *req);
void eio_rmdir(struct eio_req_t *req);
void eio_stat(struct eio_req_t *req);
void eio_symlink(struct eio_req_t *req);
void eio_truncate(struct eio_req_t *req);
void eio_unlink(struct eio_req_t *req);
void eio_utimes(struct eio_req_t *req);
void eio_write(struct eio_req_t *req);
void eio_writev(struct eio_req_t *req);

#endif // RYZE_EIO_H
