#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void Log(const FunctionCallbackInfo<Value> &args);
void ReportException(Isolate *isolate, TryCatch *try_catch);

v8::MaybeLocal<v8::Module> ResolveModuleCallback(
    v8::Local<v8::Context> context,
    v8::Local<v8::String> specifier,
    v8::Local<v8::FixedArray> import_assertions,
    v8::Local<v8::Module> referer
);

int main(int argc, char *argv[]) {
    // Initialize V8
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
#ifdef V8_ENABLE_SANDBOX
    if(!v8::V8::InitializeSandbox()) {
        printf("Error: fail to initialize the V8 sandbox\n");
        return 1;
    }
#endif
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(create_params);
    {
        v8::Isolate::Scope isolate_scope(isolate);
        // Create a stack-allocated handle scope
        v8::HandleScope handle_scope(isolate);
    
        
        v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        
        /*
        v8::Local<v8::ObjectTemplate> other = v8::ObjectTemplate::New(isolate);
        other->Set(v8::String::NewFromUtf8(isolate, "log", 
            v8::NewStringType::kNormal).ToLocalChecked(), 
            v8::FunctionTemplate::New(isolate, Log));
        global->Set(v8::String::NewFromUtf8(isolate, "other", 
            v8::NewStringType::kNormal).ToLocalChecked(), other);
        */
        
        // Create a new context
        v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global);
        
        
        /*
        v8::Local<v8::Object> globalInstance = context->Global();
        globalInstance->Set(context, v8::String::NewFromUtf8Literal(isolate, "global", 
            v8::NewStringType::kNormal), globalInstance).Check();
        */
        
        v8::Local<v8::Object> globalObj = context->Global();
        v8::Local<v8::Object> consoleObj = v8::ObjectTemplate::New(isolate)->
            NewInstance(context).ToLocalChecked();
        v8::Local<v8::Function> func = v8::Function::New(context, Log).ToLocalChecked();
        consoleObj->Set(context, v8::String::NewFromUtf8(isolate, "log")
            .ToLocalChecked(), func).ToChecked();
        globalObj->Set(context, v8::String::NewFromUtf8(isolate, "console")
            .ToLocalChecked(), consoleObj).ToChecked();
        
        
        {
            // Enter the context for compiling and running the hello world script
            v8::Context::Scope context_scope(context);

            /*
            v8::TryCatch try_catch(isolate);
            // Create a string containing the JavaScript source code
            v8::Local<v8::String> source =
                v8::String::NewFromUtf8(isolate, "console.log('hello world!');other.log('other')").ToLocalChecked();

            // Compile the source code
            v8::Local<v8::Script> script;
            if(!v8::Script::Compile(context, source).ToLocal(&script)) {
                ReportException(isolate, &try_catch);
            } else {
                v8::Local<v8::Value> result;
                if(!script->Run(context).ToLocal(&result)) {
                    ReportException(isolate, &try_catch);
                } else {
                    // Convert the result to an UTF8 string and print it
                    v8::String::Utf8Value str(isolate, result);
                    printf("%s\n", *str);
                }
            }
            */
            
            v8::Local<v8::String> resource_name = v8::String::NewFromUtf8(isolate, 
                "./main.js").ToLocalChecked();
            v8::Local<v8::PrimitiveArray> options = v8::PrimitiveArray::New(isolate, 2);
            options->Set(isolate, 0, v8::Uint32::New(isolate, 0xF1F2F3F0));
            options->Set(isolate, 1, resource_name);
            v8::ScriptOrigin origin(
                isolate,
                resource_name,
                0,
                0,
                true,
                -1,
                v8::Local<v8::Value>(),
                false,
                false,
                true,
                options
            );
            v8::Local<v8::String> source_text = v8::String::NewFromUtf8(isolate, 
                "import sayHello from './func.js'; sayHello();").ToLocalChecked();
            v8::ScriptCompiler::Source source(source_text, origin);
            v8::Local<v8::Module> module;
            if(v8::ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module)) {
                module->InstantiateModule(context, ResolveModuleCallback);
                v8::Local<v8::Value> result;
                if(module->Evaluate(context).ToLocal(&result)) {
                    v8::String::Utf8Value value(isolate, result);
                    printf("module eval result: %s\n", *value);
                }
            }
            
        }
    }
    // Dispose the isolate and tear down V8
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}

v8::MaybeLocal<v8::Module> ResolveModuleCallback(
    v8::Local<v8::Context> context,
    v8::Local<v8::String> specifier,
    v8::Local<v8::FixedArray> import_assertions,
    v8::Local<v8::Module> referer
) {
    return v8::Local<v8::Module>();
}

void Log(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::String::Utf8Value str(args.GetIsolate(), args[0]);
    printf("%s\n", *str);
    fflush(stdout);
}

void ReportException(v8::Isolate *isolate, v8::TryCatch *try_catch) {
    v8::HandleScope handle_scope(isolate);
    v8::String::Utf8Value exception(isolate, try_catch->Exception());
    v8::Local<v8::Message> message = try_catch->Message();
    if(message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; 
        // just print the exception.
        fprintf(stderr, "%s\n", *exception);
    } else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(isolate, 
            message->GetScriptOrigin().ResourceName());
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        int linenum = message->GetLineNumber(context).FromJust();
        fprintf(stderr, "%s:%i: %s\n", *filename, linenum, *exception);
        // Print line of source code.
        v8::String::Utf8Value sourceline(isolate, 
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
        v8::Local<v8::Value> stack_trace_string;
        if(try_catch->StackTrace(context).ToLocal(&stack_trace_string) 
            && stack_trace_string->IsString() 
            && stack_trace_string.As<v8::String>()->Length() > 0) {
            v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
            fprintf(stderr, "%s\n", *stack_trace);
        }
    }
}

} // namespace ryze









































































































































































































