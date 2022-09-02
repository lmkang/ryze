#include "eio.h"

void eio_access(struct eio_req_t *req) {
    if(access(req->ptr[0], req->args[0].amode)) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_chmod(struct eio_req_t *req) {
    if(chmod(req->ptr[0], req->args[0].mode)) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_chown(struct eio_req_t *req) {
    if(chown(req->ptr[0], req->args[0].owner, req->args[1].group)) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_close(struct eio_req_t *req) {
    if(close(req->args[0].fd)) {
        EIO_REQ_SETERR(req);
    }
}

void eio_fchmod(struct eio_req_t *req) {
    if(fchmod(req->args[0].fd, req->args[1].mode)) {
        EIO_REQ_SETERR(req);
    }
}

void eio_fchown(struct eio_req_t *req) {
    if(fchown(req->args[0].fd, req->args[1].owner, req->args[2].group)) {
        EIO_REQ_SETERR(req);
    }
}

void eio_fdatasync(struct eio_req_t *req) {
    if(fdatasync(req->args[0].fd)) {
        EIO_REQ_SETERR(req);
    }
}

void eio_fstat(struct eio_req_t *req) {
    if(fstat(req->args[0].fd, req->ptr[0])) {
        EIO_REQ_SETERR(req);
        free(req->ptr[0]);
        req->ptr[0] = NULL;
    }
}

void eio_fsync(struct eio_req_t *req) {
    if(fsync(req->args[0].fd)) {
        EIO_REQ_SETERR(req);
    }
}

void eio_ftruncate(struct eio_req_t *req) {
    if(ftruncate(req->args[0].fd, req->args[1].len)) {
        EIO_REQ_SETERR(req);
    }
}

void eio_futimes(struct eio_req_t *req) {
    if(futimes(req->args[0].fd, req->ptr[0])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_lchown(struct eio_req_t *req) {
    if(lchown(req->ptr[0], req->args[0].owner, req->args[1].group)) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_lutimes(struct eio_req_t *req) {
    if(lutimes(req->ptr[0], req->ptr[1])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    free(req->ptr[1]);
    req->ptr[0] = NULL;
    req->ptr[1] = NULL;
}

void eio_link(struct eio_req_t *req) {
    if(link(req->ptr[0], req->ptr[1])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    free(req->ptr[1]);
    req->ptr[0] = NULL;
    req->ptr[1] = NULL;
}

void eio_lstat(struct eio_req_t *req) {
    if(lstat(req->ptr[0], req->ptr[1])) {
        EIO_REQ_SETERR(req);
        free(req->ptr[1]);
        req->ptr[1] = NULL;
    }
    free(req->ptr[0]);
    req->ptr[0] = req->ptr[1];
    req->ptr[1] = NULL;
}

void eio_mkdir(struct eio_req_t *req) {
    if(mkdir(req->ptr[0], req->args[0].mode)) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_mkdtemp(struct eio_req_t *req) {
    if(mkdtemp(req->ptr[0]) == NULL) {
        EIO_REQ_SETERR(req);
        free(req->ptr[0]);
        req->ptr[0] = NULL;
    }
}

void eio_open(struct eio_req_t *req) {
    if((req->ret.fd = open(req->ptr[0], req->args[0].flag, req->args[0].mode)) < 0) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_opendir(struct eio_req_t *req) {
    DIR *ret = NULL;
    if((ret = opendir(req->ptr[0])) == NULL) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = ret;
}

void eio_read(struct eio_req_t *req) {
    if((req->ret.size = read(req->args[0].fd, req->ptr[0], req->args[1].size)) < 0) {
        EIO_REQ_SETERR(req);
        free(req->ptr[0]);
        req->ptr[0] = NULL;
    }
}

//struct dirent *readdir(DIR *dirp);

void eio_readlink(struct eio_req_t *req) {
    if((req->ret.size = readlink(req->ptr[0], req->ptr[1], req->args[0].size)) < 0) {
        EIO_REQ_SETERR(req);
        free(req->ptr[1]);
        req->ptr[1] = NULL;
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_readv(struct eio_req_t *req) {
    if((req->ret.size = readv(req->args[0].fd, req->ptr[0], req->args[1].count)) < 0) {
        EIO_REQ_SETERR(req);
        for(int i = 0; i < req->args[1].count; i++) {
            free(((struct iovec *) req->ptr[0])[i].iov_base);
        }
        free(req->ptr[0]);
        req->ptr[0] = NULL;
    }
}

void eio_realpath(struct eio_req_t *req) {
    if(realpath(req->ptr[0], req->ptr[1]) == NULL) {
        EIO_REQ_SETERR(req);
        free(req->ptr[1]);
        req->ptr[1] = NULL;
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_rename(struct eio_req_t *req) {
    if(rename(req->ptr[0], req->ptr[1])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    free(req->ptr[1]);
    req->ptr[0] = NULL;
    req->ptr[1] = NULL;
}

void eio_rmdir(struct eio_req_t *req) {
    if(rmdir(req->ptr[0])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_stat(struct eio_req_t *req) {
    if(stat(req->ptr[0], req->ptr[1])) {
        EIO_REQ_SETERR(req);
        free(req->ptr[1]);
        req->ptr[1] = NULL;
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_symlink(struct eio_req_t *req) {
    if(symlink(req->ptr[0], req->ptr[1])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    free(req->ptr[1]);
    req->ptr[0] = NULL;
    req->ptr[1] = NULL;
}

void eio_truncate(struct eio_req_t *req) {
    if(truncate(req->ptr[0], req->args[0].len)) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_unlink(struct eio_req_t *req) {
    if(unlink(req->ptr[0])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    req->ptr[0] = NULL;
}

void eio_utimes(struct eio_req_t *req) {
    if(utimes(req->ptr[0], req->ptr[1])) {
        EIO_REQ_SETERR(req);
    }
    free(req->ptr[0]);
    free(req->ptr[1]);
    req->ptr[0] = NULL;
    req->ptr[1] = NULL;
}

void eio_write(struct eio_req_t *req) {
    if((req->ret.size = write(req->args[0].fd, req->ptr[0], req->args[1].size)) < 0) {
        EIO_REQ_SETERR(req);
        free(req->ptr[0]);
        req->ptr[0] = NULL;
    }
}

void eio_writev(struct eio_req_t *req) {
    if((req->ret.size = writev(req->args[0].fd, req->ptr[0], req->args[1].count)) < 0) {
        EIO_REQ_SETERR(req);
        for(int i = 0; i < req->args[1].count; i++) {
            free(((struct iovec *) req->ptr[0])[i].iov_base);
        }
        free(req->ptr[0]);
        req->ptr[0] = NULL;
    }
}
