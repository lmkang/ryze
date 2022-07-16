#include <stdio.h>
#include <stdlib.h>

#include "libplatform/libplatform.h"
#include "v8.h"

namespace ryze {

using v8::Array;
using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::HandleScope;
using v8::Integer;
using v8::IntegrityLevel;
using v8::Isolate;
using v8::Just;
using v8::Local;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Module;
using v8::Nothing;
using v8::Object;
using v8::PrimitiveArray;
using v8::Promise;
using v8::ScriptCompiler;
using v8::ScriptOrigin;
using v8::String;
using v8::TryCatch;
using v8::UnboundScript;
using v8::Undefined;
using v8::Value;
using v8::Message;

enum class ModuleType {
    kJavaScript, 
    kJSON, 
    kInvalid
};

void Log(const FunctionCallbackInfo<Value> &args);
void ReportException(Isolate *isolate, TryCatch *try_catch);










void Log(const FunctionCallbackInfo<Value> &args) {
    String::Utf8Value str(args.GetIsolate(), args[0]);
    printf("%s\n", *str);
}

void ReportException(Isolate *isolate, TryCatch *try_catch) {
    HandleScope handle_scope(isolate);
    String::Utf8Value exception(isolate, try_catch->Exception());
    Local<Message> message = try_catch->Message();
    if(message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; 
        // just print the exception.
        fprintf(stderr, "%s\n", *exception);
    } else {
        // Print (filename):(line number): (message).
        String::Utf8Value filename(isolate, 
            message->GetScriptOrigin().ResourceName());
        Local<Context> context(isolate->GetCurrentContext());
        int linenum = message->GetLineNumber(context).FromJust();
        fprintf(stderr, "%s:%i: %s\n", *filename, linenum, *exception);
        // Print line of source code.
        String::Utf8Value sourceline(isolate, 
            message->GetSourceLine(context).ToLocalChecked());
        fprintf(stderr, "%s\n", *sourceline);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for(int i = 0; i < start; i++) {
            fprintf(stderr, " ");
        }
        int end = message->GetEndColumn(context).FromJust();
        for(int i = start; i < end; i++) {
            fprintf(stderr, "^");
        }
        fprintf(stderr, "\n");
        Local<Value> stack_trace_string;
        if(try_catch->StackTrace(context).ToLocal(&stack_trace_string) 
            && stack_trace_string->IsString() 
            && stack_trace_string.As<String>()->Length() > 0) {
            String::Utf8Value stack_trace(isolate, stack_trace_string);
            fprintf(stderr, "%s\n", *stack_trace);
        }
    }
}





















































































































































































































} // namespace ryze
