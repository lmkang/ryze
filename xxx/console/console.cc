#include "console.h"

void console_init(Isolate *isolate, Local<Object> obj) {
    Local<ObjectTemplate> console = ObjectTemplate::New(isolate);
    
    V8_SET_FUNC(isolate, console, "log", console_log);
    
    Local<Context> context = isolate->GetCurrentContext();
    V8_SET_OBJ(isolate, context, obj, "console", 
        console->NewInstance(context).ToLocalChecked());
}

void console_log(const FunctionCallbackInfo<Value> &args) {
    for(int i = 0; i < args.Length(); i++) {
        String::Utf8Value str(args.GetIsolate(), args[i]);
        fprintf(stdout, "%s ", *str);
    }
    fprintf(stdout, "\n");
}
