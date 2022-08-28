#include "eio.h"

struct eio_req_t *eio_req_alloc(int argc, int ret) {
    struct eio_req_t *req = malloc(sizeof(struct eio_req_t));
    if(argc > 0) {
        req->argc = argc;
        req->args = malloc(sizeof(union eio_fs_arg) * argc);
    }
    if(ret > 0) {
        req->ret = malloc(sizeof(union eio_fs_ret));
    }
    return req;
}

void eio_access(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = access(args[0].path, args[1].mode1);
    EIO_REQ_END(req);
}

void eio_chmod(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = chmod(args[0].path, args[1].mode);
    EIO_REQ_END(req);
}

void eio_chown(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = chown(args[0].path, args[1].owner, args[2].group);
    EIO_REQ_END(req);
}

void eio_close(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    printf("1. eio_close: %d\n", args[0].fd);
    ret->err = close(args[0].fd);
    printf("2. eio_close: %d\n", ret->err);
    EIO_REQ_END(req);
}

void eio_fchmod(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = fchmod(args[0].fd, args[1].mode);
    EIO_REQ_END(req);
}

void eio_fchown(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = fchown(args[0].fd, args[1].owner, args[2].group);
    EIO_REQ_END(req);
}

void eio_fdatasync(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = fdatasync(args[0].fd);
    EIO_REQ_END(req);
}

void eio_fstat(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = fstat(args[0].fd, args[1].ptr);
    EIO_REQ_END(req);
}

void eio_fsync(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = fsync(args[0].fd);
    EIO_REQ_END(req);
}

void eio_ftruncate(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = ftruncate(args[0].fd, args[1].len);
    EIO_REQ_END(req);
}

void eio_futimes(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = futimes(args[0].fd, args[1].ptr);
    EIO_REQ_END(req);
}

void eio_lchown(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = lchown(args[0].path, args[1].owner, args[2].group);
    EIO_REQ_END(req);
}

void eio_lutimes(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = lutimes(args[0].path, args[1].ptr);
    EIO_REQ_END(req);
}

void eio_link(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = link(args[0].path1, args[1].path2);
    EIO_REQ_END(req);
}

void eio_lstat(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = lstat(args[0].path, args[1].ptr);
    EIO_REQ_END(req);
}

void eio_mkdir(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = mkdir(args[0].path, args[1].mode);
    EIO_REQ_END(req);
}

void eio_mkdtemp(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_CHAR_PTR;
    ret->ptr = mkdtemp(args[0].path);
    EIO_REQ_END(req);
}

void eio_open(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    printf("1. eio_open: %s\n", (const char *) args[0].path);
    if(req->argc == 3) {
        ret->err = open(args[0].path, args[1].flag, args[2].mode);
    } else {
        ret->err = open(args[0].path, args[1].flag);
    }
    printf("2. eio_open: %d\n", ret->err);
    EIO_REQ_END(req);
}

void eio_opendir(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_DIR_PTR;
    ret->ptr = opendir(args[0].path);
    EIO_REQ_END(req);
}

void eio_read(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_SSIZE_T;
    ret->size = read(args[0].fd, args[1].buf, args[2].size);
    EIO_REQ_END(req);
}

void eio_readdir(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_DIRENT_PTR;
    ret->ptr = readdir(args[0].ptr);
    EIO_REQ_END(req);
}

void eio_readlink(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_SSIZE_T;
    ret->size = readlink(args[0].path, args[1].buf, args[2].size);
    EIO_REQ_END(req);
}

void eio_readv(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_SSIZE_T;
    ret->size = readv(args[0].fd, args[1].ptr, args[2].count);
    EIO_REQ_END(req);
}

void eio_realpath(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_CHAR_PTR;
    ret->ptr = realpath(args[0].path1, args[1].path2);
    EIO_REQ_END(req);
}

void eio_rename(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = rename(args[0].path1, args[1].path2);
    EIO_REQ_END(req);
}

void eio_rmdir(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = rmdir(args[0].path);
    EIO_REQ_END(req);
}

void eio_stat(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = stat(args[0].path, args[1].ptr);
    EIO_REQ_END(req);
}

void eio_symlink(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = symlink(args[0].path1, args[1].path2);
    EIO_REQ_END(req);
}

void eio_truncate(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = truncate(args[0].path, args[1].len);
    EIO_REQ_END(req);
}

void eio_unlink(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = unlink(args[0].path);
    EIO_REQ_END(req);
}

void eio_utimes(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_INT;
    ret->err = utimes(args[0].path, args[1].ptr);
    EIO_REQ_END(req);
}

void eio_write(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_SSIZE_T;
    ret->err = write(args[0].fd, args[1].buf, args[2].size);
    EIO_REQ_END(req);
}

void eio_writev(struct eio_req_t *req) {
    EIO_REQ_START(req);
    req->ret_type = EIO_RET_SSIZE_T;
    ret->err = writev(args[0].fd, args[1].ptr, args[2].count);
    EIO_REQ_END(req);
}
