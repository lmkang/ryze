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
    req->errnum = chmod(path, mode);
    free(path);
    return req;
}

struct eio_req_t *eio_chown(const char *path, uid_t owner, gid_t group) {
    EIO_REQ_ALLOC();
    req->errnum = chown(path, owner, group);
    free(path);
    return req;
}

struct eio_req_t *eio_close(int fd) {
    EIO_REQ_ALLOC();
    req->errnum = close(fd);
    return req;
}

struct eio_req_t *eio_fchmod(int fd, mode_t mode) {
    EIO_REQ_ALLOC();
    req->errnum = fchmod(fd, mode);
    return req;
}

struct eio_req_t *eio_fchown(int fd, uid_t owner, gid_t group) {
    EIO_REQ_ALLOC();
    req->errnum = fchown(fd, owner, group);
    return req;
}

struct eio_req_t *eio_fdatasync(int fd) {
    EIO_REQ_ALLOC();
    req->errnum = fdatasync(fd);
    return req;
}

struct eio_req_t *eio_fstat(int fd, struct stat *buf) {
    EIO_REQ_ALLOC();
    req->errnum = fstat(fd, buf);
    req->ptr[0] = buf;
    return req;
}

struct eio_req_t *eio_fsync(int fd) {
    EIO_REQ_ALLOC();
    req->errnum = fsync(fd);
    return req;
}

struct eio_req_t *eio_ftruncate(int fd, off_t length) {
    EIO_REQ_ALLOC();
    req->errnum = ftruncate(fd, length);
    return req;
}

struct eio_req_t *eio_futimes(int fd, const struct timeval tv[2]) {
    EIO_REQ_ALLOC();
    req->errnum = futimes(fd, tv);
    free(tv);
    return req;
}

struct eio_req_t *eio_lchown(const char *path, uid_t owner, gid_t group) {
    EIO_REQ_ALLOC();
    req->errnum = lchown(path, owner, group);
    free(path);
    return req;
}

struct eio_req_t *eio_lutimes(const char *filename, const struct timeval tv[2]) {
    EIO_REQ_ALLOC();
    req->errnum = lutimes(filename, tv);
    free(filename);
    free(tv);
    return req;
}

struct eio_req_t *eio_link(const char *path1, const char *path2) {
    EIO_REQ_ALLOC();
    req->errnum = link(path1, path2);
    free(path1);
    free(path2);
    return req;
}

struct eio_req_t *eio_lstat(const char *path, struct stat *buf) {
    EIO_REQ_ALLOC();
    req->errnum = lstat(path, buf);
    req->ptr[0] = buf;
    free(path);
    return req;
}

struct eio_req_t *eio_mkdir(const char *path, mode_t mode) {
    EIO_REQ_ALLOC();
    req->errnum = mkdir(path, mode);
    free(path);
    return req;
}

struct eio_req_t *eio_mkdtemp(char *template) {
    EIO_REQ_ALLOC();
    req->ptr[EIO_REQ_PTR_LEN - 1] = mkdtemp(template);
    free(template);
    return req;
}

struct eio_req_t *eio_open(const char *path, int flag, mode_t mode) {
    EIO_REQ_ALLOC();
    req->errnum = open(path, flag, mode);
    free(path);
    return req;
}

struct eio_req_t *eio_opendir(const char *dirname) {
    EIO_REQ_ALLOC();
    req->ptr[EIO_REQ_PTR_LEN - 1] = opendir(dirname);
    free(dirname);
    return req;
}

struct eio_req_t *eio_read(int fd, void *buf, size_t nbyte) {
    EIO_REQ_ALLOC();
    req->size = read(fd, buf, nbyte);
    req->ptr[0] = buf;
    return req;
}

struct eio_req_t *eio_readdir(DIR *dirp) {
    EIO_REQ_ALLOC();
    req->ptr[EIO_REQ_PTR_LEN - 1] = readdir(dirp);
    free(dirp);
    return req;
}

struct eio_req_t *eio_readlink(const char *path, char *buf, size_t bufsize) {
    EIO_REQ_ALLOC();
    req->size = readlink(path, buf, bufsize);
    req->ptr[0] = buf;
    free(path);
    return req;
}

struct eio_req_t *eio_readv(int fd, const struct iovec *iov, int iovcnt) {
    EIO_REQ_ALLOC();
    req->len = iovcnt;
    req->size = readv(fd, iov, iovcnt);
    req->ptr[0] = iov;
    return req;
}

struct eio_req_t *eio_realpath(const char *filename, char *resolved_name) {
    EIO_REQ_ALLOC();
    char *ret = realpath(filename, resolved_name);
    if(ret == NULL) {
        req->errnum = errno;
    }
    req->ptr[0] = resolved_name;
    free(filename);
    return req;
}

int rename(const char *old_name, const char *new_name) {
    EIO_REQ_ALLOC();
    req->errnum = rename(old_name, new_name);
    return req;
}










