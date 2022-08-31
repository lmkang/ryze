#include "eio.h"

struct eio_req_t *eio_access(const char *path, int amode) {
    EIO_REQ_ALLOC();
    if(access(path, amode)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_chmod(const char *path, mode_t mode) {
    EIO_REQ_ALLOC();
    if(chmod(path, mode)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_chown(const char *path, uid_t owner, gid_t group) {
    EIO_REQ_ALLOC();
    if(chown(path, owner, group)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_close(int fd) {
    EIO_REQ_ALLOC();
    if(close(fd)) {
        EIO_REQ_SETERR(req);
    }
    return req;
}

struct eio_req_t *eio_fchmod(int fd, mode_t mode) {
    EIO_REQ_ALLOC();
    if(fchmod(fd, mode)) {
        EIO_REQ_SETERR(req);
    }
    return req;
}

struct eio_req_t *eio_fchown(int fd, uid_t owner, gid_t group) {
    EIO_REQ_ALLOC();
    if(fchown(fd, owner, group)) {
        EIO_REQ_SETERR(req);
    }
    return req;
}

struct eio_req_t *eio_fdatasync(int fd) {
    EIO_REQ_ALLOC();
    if(fdatasync(fd)) {
        EIO_REQ_SETERR(req);
    }
    return req;
}

struct eio_req_t *eio_fstat(int fd, struct stat *buf) {
    EIO_REQ_ALLOC();
    if(fstat(fd, buf)) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = buf;
    return req;
}

struct eio_req_t *eio_fsync(int fd) {
    EIO_REQ_ALLOC();
    if(fsync(fd)) {
        EIO_REQ_SETERR(req);
    }
    return req;
}

struct eio_req_t *eio_ftruncate(int fd, off_t length) {
    EIO_REQ_ALLOC();
    if(ftruncate(fd, length)) {
        EIO_REQ_SETERR(req);
    }
    return req;
}

struct eio_req_t *eio_futimes(int fd, const struct timeval tv[2]) {
    EIO_REQ_ALLOC();
    if(futimes(fd, tv)) {
        EIO_REQ_SETERR(req);
    }
    free(tv);
    return req;
}

struct eio_req_t *eio_lchown(const char *path, uid_t owner, gid_t group) {
    EIO_REQ_ALLOC();
    if(lchown(path, owner, group)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_lutimes(const char *filename, const struct timeval tv[2]) {
    EIO_REQ_ALLOC();
    if(lutimes(filename, tv)) {
        EIO_REQ_SETERR(req);
    }
    free(filename);
    free(tv);
    return req;
}

struct eio_req_t *eio_link(const char *path1, const char *path2) {
    EIO_REQ_ALLOC();
    if(link(path1, path2)) {
        EIO_REQ_SETERR(req);
    }
    free(path1);
    free(path2);
    return req;
}

struct eio_req_t *eio_lstat(const char *path, struct stat *buf) {
    EIO_REQ_ALLOC();
    if(lstat(path, buf)) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = buf;
    free(path);
    return req;
}

struct eio_req_t *eio_mkdir(const char *path, mode_t mode) {
    EIO_REQ_ALLOC();
    if(mkdir(path, mode)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_mkdtemp(char *template) {
    EIO_REQ_ALLOC();
    if((req->retbuf = mkdtemp(template)) == NULL) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = template;
    return req;
}

struct eio_req_t *eio_open(const char *path, int flag, mode_t mode) {
    EIO_REQ_ALLOC();
    if((req->errnum = open(path, flag, mode)) < 0) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_opendir(const char *dirname) {
    EIO_REQ_ALLOC();
    if((req->retbuf = opendir(dirname)) == NULL) {
        EIO_REQ_SETERR(req);
    }
    free(dirname);
    return req;
}

struct eio_req_t *eio_read(int fd, void *buf, size_t nbyte) {
    EIO_REQ_ALLOC();
    if((req->size = read(fd, buf, nbyte)) < 0) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = buf;
    return req;
}

struct eio_req_t *eio_readdir(DIR *dirp) {
    EIO_REQ_ALLOC();
    if((req->retbuf = readdir(dirp)) == NULL) {
        EIO_REQ_SETERR(req);
    }
    free(dirp);
    return req;
}

struct eio_req_t *eio_readlink(const char *path, char *buf, size_t bufsize) {
    EIO_REQ_ALLOC();
    if((req->size = readlink(path, buf, bufsize)) < 0) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = buf;
    free(path);
    return req;
}

struct eio_req_t *eio_readv(int fd, const struct iovec *iov, int iovcnt) {
    EIO_REQ_ALLOC();
    if((req->size = readv(fd, iov, iovcnt)) < 0) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = iov;
    req->count = iovcnt;
    return req;
}

struct eio_req_t *eio_realpath(const char *filename, char *resolved_name) {
    EIO_REQ_ALLOC();
    if((req->retbuf = realpath(filename, resolved_name)) == NULL) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = resolved_name;
    free(filename);
    return req;
}

struct eio_req_t *eio_rename(const char *old_name, const char *new_name) {
    EIO_REQ_ALLOC();
    if(rename(old_name, new_name)) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = new_name;
    free(old_name);
    return req;
}

struct eio_req_t *eio_rmdir(const char *path) {
    EIO_REQ_ALLOC();
    if(rmdir(path)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_stat(const char *path, struct stat *buf) {
    EIO_REQ_ALLOC();
    if(stat(path, buf)) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = buf;
    free(path);
    return req;
}

struct eio_req_t *eio_symlink(const char *path1, const char *path2) {
    EIO_REQ_ALLOC();
    if(symlink(path1, path2)) {
        EIO_REQ_SETERR(req);
    }
    free(path1);
    free(path2);
    return req;
}

struct eio_req_t *eio_truncate(const char *path, off_t length) {
    EIO_REQ_ALLOC();
    if(truncate(path, length)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_unlink(const char *path) {
    EIO_REQ_ALLOC();
    if(unlink(path)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    return req;
}

struct eio_req_t *eio_utimes(const char *path, const struct timeval tv[2]) {
    EIO_REQ_ALLOC();
    if(utimes(path, tv)) {
        EIO_REQ_SETERR(req);
    }
    free(path);
    free(tv);
    return req;
}

struct eio_req_t *eio_write(int fd, const void *buf, size_t nbyte) {
    EIO_REQ_ALLOC();
    if((req->size = write(fd, buf, nbyte)) < 0) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = buf;
    return req;
}

struct eio_req_t *eio_writev(int fd, const struct iovec *iov, int iovcnt) {
    EIO_REQ_ALLOC();
    if((req->size = writev(fd, iov, iovcnt)) < 0) {
        EIO_REQ_SETERR(req);
    }
    req->ptr[0] = iov;
    req->count = iovcnt;
    return req;
}
