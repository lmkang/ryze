#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <string>
#include <vector>
#include <iomanip>
#include <iterator>
#include <unordered_map>

#include "libplatform/libplatform.h"
#include "v8.h"

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::ObjectTemplate;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Module;
using v8::Object;
using v8::PrimitiveArray;
using v8::Promise;
using v8::ScriptCompiler;
using v8::ScriptOrigin;
using v8::String;
using v8::NewStringType;
using v8::TryCatch;
using v8::Value;
using v8::Message;
using v8::FixedArray;

MaybeLocal<String> ReadFile(Isolate *isolate, const std::string path);
void ReportException(Isolate *isolate, TryCatch *try_catch);
MaybeLocal<Module> ResolveModuleCallback(
    Local<Context> context,
    Local<String> specifier,
    Local<FixedArray> import_assertions,
    Local<Module> referer);
void HostInitializeImportMetaObject(
    Local<Context> context,
    Local<Module> module,
    Local<Object> meta);

void Log(const FunctionCallbackInfo<Value> &args);
void DirName(const FunctionCallbackInfo<Value> &args);
void Join(const FunctionCallbackInfo<Value> &args);
void Read(const FunctionCallbackInfo<Value> &args);

std::string g_cwd;
std::unordered_map<int, std::string> module_to_specifier_map;

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
    char *cwd = new char(255);
    getcwd(cwd, 255);
    g_cwd = cwd;
    
    // Create a new Isolate and make it the current one
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate *isolate = Isolate::New(create_params);
    isolate->SetHostInitializeImportMetaObjectCallback(HostInitializeImportMetaObject);
    {
        Isolate::Scope isolate_scope(isolate);
        // Create a stack-allocated handle scope
        HandleScope handle_scope(isolate);
    
        Local<ObjectTemplate> global_tmpl = ObjectTemplate::New(isolate);
        
        Local<ObjectTemplate> fs_tmpl = ObjectTemplate::New(isolate);
        fs_tmpl->Set(String::NewFromUtf8(isolate, "read", NewStringType::kNormal)
            .ToLocalChecked(), FunctionTemplate::New(isolate, Read));
        global_tmpl->Set(String::NewFromUtf8(isolate, "fs", 
            NewStringType::kNormal).ToLocalChecked(), fs_tmpl);
        
        Local<ObjectTemplate> path_tmpl = ObjectTemplate::New(isolate);
        path_tmpl->Set(String::NewFromUtf8(isolate, "dirname", NewStringType::kNormal)
            .ToLocalChecked(), FunctionTemplate::New(isolate, DirName));
        path_tmpl->Set(String::NewFromUtf8(isolate, "join", NewStringType::kNormal)
            .ToLocalChecked(), FunctionTemplate::New(isolate, Join));
        global_tmpl->Set(String::NewFromUtf8(isolate, "path", 
            NewStringType::kNormal).ToLocalChecked(), path_tmpl);
        
        // Create a new context
        Local<Context> context = Context::New(isolate, NULL, global_tmpl);
        
        /*
        Local<Object> globalInstance = context->Global();
        globalInstance->Set(context, String::NewFromUtf8Literal(isolate, "global", 
            NewStringType::kNormal), globalInstance).Check();
        */
        
        Local<Object> globalObj = context->Global();
        Local<Object> consoleObj = ObjectTemplate::New(isolate)->
            NewInstance(context).ToLocalChecked();
        consoleObj->Set(context, String::NewFromUtf8(isolate, "log").ToLocalChecked(), 
            Function::New(context, Log).ToLocalChecked()).ToChecked();
        globalObj->Set(context, String::NewFromUtf8(isolate, "console")
            .ToLocalChecked(), consoleObj).ToChecked();
        
        {
            // Enter the context for compiling and running the hello world script
            Context::Scope context_scope(context);
            Local<String> resource_name = String::NewFromUtf8(
                isolate, "./js/main.js").ToLocalChecked();
            Local<PrimitiveArray> options = PrimitiveArray::New(isolate, 2);
            options->Set(isolate, 0, v8::Uint32::New(isolate, 0xF1F2F3F0));
            options->Set(isolate, 1, resource_name);
            ScriptOrigin origin(
                isolate,
                resource_name,
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
            String::Utf8Value file_path(isolate, resource_name);
            MaybeLocal<String> source_text = ReadFile(isolate, *file_path);
            ScriptCompiler::Source source(source_text.ToLocalChecked(), origin);
            Local<Module> module;
            if(ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module)) {
                module_to_specifier_map.insert(std::make_pair(
                    module->GetIdentityHash(), *file_path));
                if(module->InstantiateModule(context, 
                    ResolveModuleCallback).FromMaybe(false)) {
                    Local<Value> result;
                    if(module->Evaluate(context).ToLocal(&result)) {
                        Local<Promise> result_promise(result.As<Promise>());
                        while(result_promise->State() == Promise::kPending) {
                            // Loop until module execution finishes
                        }
                    }
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

MaybeLocal<Module> ResolveModuleCallback(
    Local<Context> context,
    Local<String> specifier,
    Local<FixedArray> import_assertions,
    Local<Module> referer) {
    
    Isolate *isolate = context->GetIsolate();
    String::Utf8Value path(isolate, specifier);
    
    auto specifier_it = module_to_specifier_map.find(referer->GetIdentityHash());
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
    options->Set(isolate, 0, v8::Uint32::New(isolate, 0xF1F2F3F0));
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
        module_to_specifier_map.insert(std::make_pair(
        module->GetIdentityHash(), absolute_path));
        return module;
    }
    return MaybeLocal<Module>();
}

void HostInitializeImportMetaObject(
    Local<Context> context,
    Local<Module> module,
    Local<Object> meta) {
    
    Isolate *isolate = context->GetIsolate();
    HandleScope handle_scope(isolate);
    auto specifier_it = module_to_specifier_map.find(module->GetIdentityHash());
    Local<String> url_key =String::NewFromUtf8Literal(isolate, "url", 
        NewStringType::kInternalized);
    Local<String> dirname_key =String::NewFromUtf8Literal(isolate, "dirname", 
        NewStringType::kInternalized);
    std::string absolute_path = g_cwd + "/" + specifier_it->second;
    Local<String> url = String::NewFromUtf8(isolate, 
        absolute_path.c_str()).ToLocalChecked();
    size_t index = absolute_path.find_last_of('/');
    std::string dir;
    if(index != std::string::npos) {
        dir = absolute_path.substr(0, index);
    }
    Local<String> dirname = String::NewFromUtf8(isolate, 
        dir.c_str()).ToLocalChecked();
    meta->CreateDataProperty(context, url_key, url).ToChecked();
    meta->CreateDataProperty(context, dirname_key, dirname).ToChecked();
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

void Log(const FunctionCallbackInfo<Value> &args) {
    for(int i = 0; i < args.Length(); i++) {
        String::Utf8Value str(args.GetIsolate(), args[i]);
        fprintf(stdout, "%s ", *str);
    }
    fprintf(stdout, "\n");
}

void DirName(const FunctionCallbackInfo<Value> &args) {
    String::Utf8Value str(args.GetIsolate(), args[0]);
    std::string path = *str;
    size_t index = path.find_last_of('/');
    std::string dir;
    if(index != std::string::npos) {
        dir = path.substr(0, index);
    }
    args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), dir.c_str(), 
        NewStringType::kNormal, static_cast<int>(dir.size())).ToLocalChecked());
}

void Join(const FunctionCallbackInfo<Value> &args) {
    std::string path("");
    std::string seg;
    for(int i = 0; i < args.Length(); i++) {
        String::Utf8Value str(args.GetIsolate(), args[i]);
        seg = *str;
        if(i == 0) {
            size_t size = seg.size();
            if(seg[size -1] == '/') {
                seg = seg.substr(0, size - 1);
            }
            path.append(seg);
        } else {
            if(seg[0] == '/') {
                path.append(seg.substr(1));
            } else {
                path.append("/").append(seg);
            }
        }
    }
    args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), path.c_str(), 
        NewStringType::kNormal, static_cast<int>(path.size())).ToLocalChecked());
}

void Read(const FunctionCallbackInfo<Value> &args) {
    String::Utf8Value path(args.GetIsolate(), args[0]);
    Local<String> value;
    if(!ReadFile(args.GetIsolate(), *path).ToLocal(&value)) {
        args.GetIsolate()->ThrowError("Error loading file");
        return;
    }
    args.GetReturnValue().Set(value);
}






































