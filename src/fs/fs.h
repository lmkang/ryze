#ifndef RYZE_FS_H
#define RYZE_FS_H

#include "common.h"

extern "C" {
    #include "ev.h"
}

void fs_init(Isolate *isolate, Local<Object> obj);
void fs_open(const FunctionCallbackInfo<Value> &args);
void fs_read(const FunctionCallbackInfo<Value> &args);
void fs_close(const FunctionCallbackInfo<Value> &args);

#endif // RYZE_FS_H
