#include "epoll.h"

void cattle::epoll::EpollCreat(const FunctionCallbackInfo<Value> &args) {
    int ret = epoll_create1(Local<Integer>::Cast(args[0])->Value());
    args.GetReturnValue().Set(Integer::New(args.GetIsolate(), ret));
}

void cattle::epoll::EpollCtl(const FunctionCallbackInfo<Value> &args) {
    int epfd = Local<Integer>::Cast(args[0])->Value();
    int action = Local<Integer>::Cast(args[1])->Value();
    int fd = Local<Integer>::Cast(args[2])->Value();
    int mask = Local<Integer>::Cast(args[3])->Value();
    struct epoll_event ev;
    ev.events = mask;
    ev.data.fd = fd;
    int ret = epoll_ctl(evfd, action, fd, &ev);
    args.GetReturnValue().Set(Integer::New(args.GetIsolate(), ret));
}



