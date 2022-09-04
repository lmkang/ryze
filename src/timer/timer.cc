#include "timer.h"

void timer_init(Isolate *isolate, Local<Object> obj) {
    V8_SET_FUNC(isolate, obj, "setTimeout", timer_set_timeout);
    V8_SET_FUNC(isolate, obj, "setInterval", timer_set_interval);
    V8_SET_FUNC(isolate, obj, "clearTimeout", timer_clear_timeout);
    V8_SET_FUNC(isolate, obj, "clearInterval", timer_clear_interval);
}

void timer_set_timeout(const FunctionCallbackInfo<Value> &args) {
    
}

void timer_set_interval(const FunctionCallbackInfo<Value> &args) {
    
}

void timer_clear_timeout(const FunctionCallbackInfo<Value> &args) {
    
}

void timer_clear_interval(const FunctionCallbackInfo<Value> &args) {
    
}
