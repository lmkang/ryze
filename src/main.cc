#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <vector>
#include <iomanip>
#include <iterator>
#include <unordered_map>
#include "common.h"

extern "C" {
    #include "ev.h"
}

#define TO_UTF8(isolate, str) \
    String::NewFromUtf8(isolate, str).ToLocalChecked()

void LoopCallback(struct eio_req_t *req);
MaybeLocal<Module> ResolveModuleCallback(
    Local<Context> context,
    Local<String> specifier,
    Local<FixedArray> import_assertions,
    Local<Module> referer);
void InitImportMetaObjectCallback(
    Local<Context> context,
    Local<Module> module,
    Local<Object> meta);
std::string NormalizePath(const std::string &path, const std::string &dir);
MaybeLocal<String> ReadFile(Isolate *isolate, const std::string path);
void ReportException(Isolate *isolate, TryCatch *try_catch);

void js_log(const FunctionCallbackInfo<Value> &args);
void js_open(const FunctionCallbackInfo<Value> &args);
void js_close(const FunctionCallbackInfo<Value> &args);

std::unordered_map<int, std::string> module_specifier_map;
struct ev_loop_t *loop = NULL;

int main(int argc, char **argv) {
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    #ifdef V8_ENABLE_SANDBOX
    if(!v8::V8::InitializeSandbox()) {
        v8::V8::Dispose();
        v8::V8::DisposePlatform();
        printf("Error: fail to initialize the V8 sandbox\n");
        return 1;
    }
    #endif
    v8::V8::Initialize();
    
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate *isolate = Isolate::New(create_params);
    isolate->SetHostInitializeImportMetaObjectCallback(InitImportMetaObjectCallback);
    
    loop = ev_loop_init();
    { // isolate_scope
    
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    
    Local<Context> context = Context::New(isolate, NULL, ObjectTemplate::New(isolate));
    Local<Object> g_obj = context->Global();
    // console
    Local<Object> console_obj = ObjectTemplate::New(isolate)->NewInstance(context)
        .ToLocalChecked();
    console_obj->Set(context, TO_UTF8(isolate, "log"), Function::New(context, js_log)
        .ToLocalChecked()).ToChecked();
    // fs
    Local<Object> fs_obj = ObjectTemplate::New(isolate)->NewInstance(context)
        .ToLocalChecked();
    fs_obj->Set(context, TO_UTF8(isolate, "open"), Function::New(context, js_open)
        .ToLocalChecked()).ToChecked();
    fs_obj->Set(context, TO_UTF8(isolate, "close"), Function::New(context, js_close)
        .ToLocalChecked()).ToChecked();
    g_obj->Set(context, TO_UTF8(isolate, "console"), console_obj).ToChecked();
    g_obj->Set(context, TO_UTF8(isolate, "fs"), fs_obj).ToChecked();
    
    Context::Scope context_scope(context);
    Local<String> js_file = TO_UTF8(isolate,  "/home/share/ryze/js/main.js");
    Local<PrimitiveArray> options = PrimitiveArray::New(isolate, 2);
    options->Set(isolate, 0, v8::Uint32::New(isolate, 0x135e0619));
    options->Set(isolate, 1, js_file);
    ScriptOrigin origin(
        isolate,
        js_file,
        0,
        0,
        true,
        -1,
        Local<Value>(),
        false,
        false,
        true,
        options
    );
    String::Utf8Value path(isolate, js_file);
    MaybeLocal<String> src_text = ReadFile(isolate, *path);
    ScriptCompiler::Source source(src_text.ToLocalChecked(), origin);
    Local<Module> module;
    if(ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module)) {
        module_specifier_map.insert(std::make_pair(module->GetIdentityHash(), *path));
        if(module->InstantiateModule(context, ResolveModuleCallback).FromMaybe(false)) {
            Local<Value> result;
            if(module->Evaluate(context).ToLocal(&result)) {
                Local<Promise> promise(result.As<Promise>());
                while(promise->State() == Promise::kPending) {
                    // Loop until module execution finish
                }
            }
        }
    }
    ev_loop_run(loop, LoopCallback);
    ev_loop_free(loop);
    
    } // isolate_scope
    
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}

void LoopCallback(struct eio_req_t *req) {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Global<Promise::Resolver> *r = (Global<Promise::Resolver> *) req->resolver;
    auto resolver = Local<Promise::Resolver>::New(isolate, *r);
    union eio_fs_ret *ret = (union eio_fs_ret *) req->ret;
    auto result = resolver->Resolve(context, Integer::New(isolate, ret->err));
    delete r;
}

MaybeLocal<Module> ResolveModuleCallback(
        Local<Context> context,
        Local<String> specifier,
        Local<FixedArray> import_assertions,
        Local<Module> referer) {
    Isolate *isolate = context->GetIsolate();
    String::Utf8Value path(isolate, specifier);
    auto specifier_it = module_specifier_map.find(referer->GetIdentityHash());
    std::string str = specifier_it->second;
    std::string dir("");
    size_t index = str.find_last_of('/');
    if(index != std::string::npos) {
        dir = str.substr(0, index);
    }
    std::string absolute_path = NormalizePath(*path, dir);
    MaybeLocal<String> source_text = ReadFile(isolate, absolute_path);
    if(source_text.IsEmpty()) {
        printf("Error reading  module from %s\n", *path);
        return MaybeLocal<Module>();
    }
    Local<PrimitiveArray> options = PrimitiveArray::New(isolate, 2);
    options->Set(isolate, 0, v8::Uint32::New(isolate, 0x135e0511));
    options->Set(isolate, 1, specifier);
    ScriptOrigin origin(
        isolate,
        specifier,
        0,
        0,
        true,
        -1,
        Local<Value>(),
        false,
        false,
        true,
        options
    );
    ScriptCompiler::Source source(source_text.ToLocalChecked(), origin);
    Local<Module> module;
    if(ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module)) {
        module_specifier_map.insert(std::make_pair(
        module->GetIdentityHash(), absolute_path));
        return module;
    }
    return MaybeLocal<Module>();
}

void InitImportMetaObjectCallback(
        Local<Context> context,
        Local<Module> module,
        Local<Object> meta) {
    Isolate *isolate = context->GetIsolate();
    HandleScope handle_scope(isolate);
    auto specifier_it = module_specifier_map.find(module->GetIdentityHash());
    Local<String> url_key = TO_UTF8(isolate, "url");
    char cwd[255];
    getcwd(cwd, 255);
    std::string absolute_path = (std::string) cwd + "/" + specifier_it->second;
    Local<String> url = TO_UTF8(isolate, absolute_path.c_str());
    meta->CreateDataProperty(context, url_key, url).ToChecked();
}

std::string NormalizePath(const std::string &path, const std::string &dir) {
    std::string absolute_path;
    if(path[0] == '/') {
        absolute_path = path;
    } else {
        absolute_path = dir + '/' + path;
    }
    std::vector<std::string> segments;
    std::istringstream segment_stream(absolute_path);
    std::string segment;
    while(std::getline(segment_stream, segment, '/')) {
        if(segment == "..") {
            if(!segments.empty()) {
                segments.pop_back();
            }
        } else if(segment != ".") {
            segments.push_back(segment);
        }
    }
    // Join path segments.
    std::ostringstream os;
    if(segments.size() > 1) {
        std::copy(segments.begin(), segments.end() - 1,
            std::ostream_iterator<std::string>(os, "/"));
        os << *segments.rbegin();
    } else {
        os << "/";
        if(!segments.empty()) {
            os << segments[0];
        }
    }
    return os.str();
}

MaybeLocal<String> ReadFile(Isolate *isolate, const std::string path) {
    FILE *file = fopen(path.c_str(), "rb");
    if(file == NULL) {
        return MaybeLocal<String>();
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    char *chars = new char[size + 1];
    chars[size] = '\0';
    for(size_t i = 0; i < size; ) {
        i += fread(&chars[i], 1, size - i, file);
        if(ferror(file)) {
            fclose(file);
            return MaybeLocal<String>();
        }
    }
    fclose(file);
    MaybeLocal<String> result = String::NewFromUtf8(isolate, chars, 
        NewStringType::kNormal, static_cast<int>(size));
    delete[] chars;
    return result;
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

void js_log(const FunctionCallbackInfo<Value> &args) {
    for(int i = 0; i < args.Length(); i++) {
        String::Utf8Value str(args.GetIsolate(), args[i]);
        fprintf(stdout, "%s ", *str);
    }
    fprintf(stdout, "\n");
}

void eio_req_path_free(struct eio_req_t *req) {
    free(((union eio_fs_arg *) req->args)[0].path);
    printf("eio_req_path_free\n");
}

void js_open(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    struct eio_req_t *req = eio_req_alloc(3, 1);
    union eio_fs_arg *fs_args = (union eio_fs_arg *) req->args;
    String::Utf8Value str(isolate, args[0]);
    int len = str.length();
    char *path = (char *) malloc(sizeof(char) * (len + 1));
    path[len] = '\0';
    memcpy(path, *str, len);
    fs_args[0].path = path;
    fs_args[1].flag = O_RDONLY;
    fs_args[2].mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    req->work = eio_open;
    req->release = eio_req_path_free;
    
    Local<Promise::Resolver> resolver = Promise::Resolver::New(context).ToLocalChecked();
    Global<Promise::Resolver> *p = new Global<Promise::Resolver>();
    p->Reset(isolate, resolver);
    req->resolver = p;
    ev_req_add(loop, req);
    
    args.GetReturnValue().Set(resolver->GetPromise());
}

void js_read(const FunctionCallbackInfo<Value> &args) {
    
}

void js_close(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    
    struct eio_req_t *req = eio_req_alloc(1, 1);
    union eio_fs_arg *fs_args = (union eio_fs_arg *) req->args;
    fs_args[0].fd = Local<Integer>::Cast(args[0])->Value();
    req->work = eio_close;
    
    Local<Promise::Resolver> resolver = Promise::Resolver::New(context).ToLocalChecked();
    Global<Promise::Resolver> *p = new Global<Promise::Resolver>();
    p->Reset(isolate, resolver);
    req->resolver = p;
    ev_req_add(loop, req);
    
    args.GetReturnValue().Set(resolver->GetPromise());
}
