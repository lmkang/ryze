#ifndef RYZE_TIMER_H
#define RYZE_TIMER_H

#include "common.h"

void timer_init(Isolate *isolate, Local<Object> obj);
void timer_set_timeout(const FunctionCallbackInfo<Value> &args);
void timer_set_interval(const FunctionCallbackInfo<Value> &args);
void timer_clear_timeout(const FunctionCallbackInfo<Value> &args);
void timer_clear_interval(const FunctionCallbackInfo<Value> &args);

#endif // RYZE_TIMER_H
