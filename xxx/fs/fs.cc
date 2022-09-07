#include "fs.h"

void fs_init(Isolate *isolate, Local<Object> obj) {
    Local<ObjectTemplate> fs = ObjectTemplate::New(isolate);
    
    V8_SET_FUNC(isolate, fs, "open", fs_open);
    V8_SET_FUNC(isolate, fs, "read", fs_read);
    V8_SET_FUNC(isolate, fs, "close", fs_close);
    
    V8_SET_VALUE(isolate, fs, "O_RDONLY", V8_INT(isolate, O_RDONLY));
    V8_SET_VALUE(isolate, fs, "O_WRONLY", V8_INT(isolate, O_WRONLY));
    V8_SET_VALUE(isolate, fs, "O_RDWR", V8_INT(isolate, O_RDWR));
    V8_SET_VALUE(isolate, fs, "O_CREAT", V8_INT(isolate, O_CREAT));
    V8_SET_VALUE(isolate, fs, "O_EXCL", V8_INT(isolate, O_EXCL));
    V8_SET_VALUE(isolate, fs, "O_APPEND", V8_INT(isolate, O_APPEND));
    V8_SET_VALUE(isolate, fs, "O_SYNC", V8_INT(isolate, O_SYNC));
    V8_SET_VALUE(isolate, fs, "O_TRUNC", V8_INT(isolate, O_TRUNC));
    
    V8_SET_VALUE(isolate, fs, "S_IRUSR", V8_INT(isolate, S_IRUSR));
    V8_SET_VALUE(isolate, fs, "S_IWUSR", V8_INT(isolate, S_IWUSR));
    V8_SET_VALUE(isolate, fs, "S_IXUSR", V8_INT(isolate, S_IXUSR));
    V8_SET_VALUE(isolate, fs, "S_IRGRP", V8_INT(isolate, S_IRGRP));
    V8_SET_VALUE(isolate, fs, "S_IWGRP", V8_INT(isolate, S_IWGRP));
    V8_SET_VALUE(isolate, fs, "S_IXGRP", V8_INT(isolate, S_IXGRP));
    V8_SET_VALUE(isolate, fs, "S_IROTH", V8_INT(isolate, S_IROTH));
    V8_SET_VALUE(isolate, fs, "S_IWOTH", V8_INT(isolate, S_IWOTH));
    V8_SET_VALUE(isolate, fs, "S_IXOTH", V8_INT(isolate, S_IXOTH));
    V8_SET_VALUE(isolate, fs, "S_IRWXO", V8_INT(isolate, S_IRWXO));
    V8_SET_VALUE(isolate, fs, "S_IRWXG", V8_INT(isolate, S_IRWXG));
    V8_SET_VALUE(isolate, fs, "S_IRWXU", V8_INT(isolate, S_IRWXU));
    
    Local<Context> context = isolate->GetCurrentContext();
    V8_SET_OBJ(isolate, context, obj, "fs", fs->NewInstance(context).ToLocalChecked());
}

// case 1: return errnum, no arg
void eio_req_fs_callback1(struct eio_req_t *req) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    Global<Promise::Resolver> *r = (Global<Promise::Resolver> *) req->resolver;
    auto resolver = Local<Promise::Resolver>::New(isolate, *r);
    if(req->errnum) {
        Local<Object> obj = Object::New(isolate);
        obj->Set(context, V8_STR(isolate, "errno"), V8_INT(isolate, req->errnum)).ToChecked();
        obj->Set(context, V8_STR(isolate, "error"), V8_STR(isolate, req->errbuf)).ToChecked();
        auto result = resolver->Reject(context, obj);
    } else {
        auto result = resolver->Resolve(context, V8_INT(isolate, req->errnum));
    }
    delete r;
}

// case 2: return fd, no arg
void eio_req_fs_callback2(struct eio_req_t *req) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    Global<Promise::Resolver> *r = (Global<Promise::Resolver> *) req->resolver;
    auto resolver = Local<Promise::Resolver>::New(isolate, *r);
    if(req->errnum) {
        Local<Object> obj = Object::New(isolate);
        obj->Set(context, V8_STR(isolate, "errno"), V8_INT(isolate, req->errnum)).ToChecked();
        obj->Set(context, V8_STR(isolate, "error"), V8_STR(isolate, req->errbuf)).ToChecked();
        auto result = resolver->Reject(context, obj);
    } else {
        auto result = resolver->Resolve(context, V8_INT(isolate, req->ret.fd));
    }
    delete r;
}

// case 3: return size, 1 arg
void eio_req_fs_callback3(struct eio_req_t *req) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    Global<Promise::Resolver> *r = (Global<Promise::Resolver> *) req->resolver;
    auto resolver = Local<Promise::Resolver>::New(isolate, *r);
    if(req->errnum) {
        Local<Object> obj = Object::New(isolate);
        obj->Set(context, V8_STR(isolate, "errno"), V8_INT(isolate, req->errnum)).ToChecked();
        obj->Set(context, V8_STR(isolate, "error"), V8_STR(isolate, req->errbuf)).ToChecked();
        auto result = resolver->Reject(context, obj);
    } else {
        auto result = resolver->Resolve(context, V8_STR(isolate, (char *) req->ptr[0]));
    }
    delete r;
}

void fs_open(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    String::Utf8Value str(isolate, args[0]);
    int len = str.length();
    char *path = (char *) malloc(sizeof(char) * (len + 1));
    path[len] = '\0';
    memcpy(path, *str, len);
    EIO_REQ_ALLOC();
    req->ptr[0] = path;
    req->args[0].flag = O_RDONLY;
    req->args[1].mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    len = args.Length();
    if(len >= 2) {
        req->args[0].flag = Local<Integer>::Cast(args[1])->Value();
    }
    if(len >= 3) {
        req->args[1].mode = Local<Integer>::Cast(args[2])->Value();
    }
    req->work = eio_open;
    req->callback = eio_req_fs_callback2;
    
    Local<Promise::Resolver> resolver = Promise::Resolver::New(context).ToLocalChecked();
    Global<Promise::Resolver> *p = new Global<Promise::Resolver>();
    p->Reset(isolate, resolver);
    req->resolver = p;
    struct ev_loop_t *loop = 
        (struct ev_loop_t *) context->GetAlignedPointerFromEmbedderData(0);
    EV_REQ_ADD(loop, req);
    
    args.GetReturnValue().Set(resolver->GetPromise());
}

void fs_read(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    EIO_REQ_ALLOC();
    req->args[0].fd = Local<Integer>::Cast(args[0])->Value();
    req->args[1].size = 255;
    req->ptr[0] = malloc(sizeof(char) * req->args[1].size);
    req->work = eio_read;
    req->callback = eio_req_fs_callback3;
    Local<Promise::Resolver> resolver = Promise::Resolver::New(context).ToLocalChecked();
    Global<Promise::Resolver> *p = new Global<Promise::Resolver>();
    p->Reset(isolate, resolver);
    req->resolver = p;
    struct ev_loop_t *loop = 
        (struct ev_loop_t *) context->GetAlignedPointerFromEmbedderData(0);
    EV_REQ_ADD(loop, req);
    
    args.GetReturnValue().Set(resolver->GetPromise());
}

void fs_close(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    EIO_REQ_ALLOC();
    req->args[0].fd = Local<Integer>::Cast(args[0])->Value();
    req->work = eio_close;
    req->callback = eio_req_fs_callback1;
    
    Local<Promise::Resolver> resolver = Promise::Resolver::New(context).ToLocalChecked();
    Global<Promise::Resolver> *p = new Global<Promise::Resolver>();
    p->Reset(isolate, resolver);
    req->resolver = p;
    struct ev_loop_t *loop = 
        (struct ev_loop_t *) context->GetAlignedPointerFromEmbedderData(0);
    EV_REQ_ADD(loop, req);
    
    args.GetReturnValue().Set(resolver->GetPromise());
}
