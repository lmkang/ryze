#include "console.h"

void console_init(Isolate *isolate) {
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> gobj = context->Global();
    Local<ObjectTemplate> obj_tmpl = ObjectTemplate::New(isolate);
    V8_SET_FUNC(isolate, obj_tmpl, "log", console_log);
    V8_SET_OBJ(isolate, gobj, "console", obj_tmpl);
}

void console_log(const FunctionCallbackInfo<Value> &args) {
    for(int i = 0; i < args.Length(); i++) {
        String::Utf8Value str(args.GetIsolate(), args[i]);
        fprintf(stdout, "%s ", *str);
    }
    fprintf(stdout, "\n");
}
