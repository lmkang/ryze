#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <iomanip>
#include <iterator>
#include <unordered_map>
#include "common.h"

namespace {

std::unordered_map<int, std::string> module_specifier_map;

char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        printf("Error: can not open file: %s\n", path);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    char *str = (char *) malloc(size + 1);
    str[size] = '\0';
    for(size_t i = 0; i < size; ) {
        i += fread(str + i, 1, size - i, fp);
        if(ferror(fp)) {
            fclose(fp);
            free(str);
            return NULL;
        }
    }
    fclose(fp);
    return str;
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

void InitImportMetaObjectCallback(
        Local<Context> context,
        Local<Module> module,
        Local<Object> meta) {
    Isolate *isolate = context->GetIsolate();
    HandleScope handle_scope(isolate);
    auto specifier_it = module_specifier_map.find(module->GetIdentityHash());
    Local<String> url_key = String::NewFromUtf8Literal(isolate, "url", 
        NewStringType::kInternalized);
    char cwd[255];
    getcwd(cwd, 255);
    std::string str = (std::string) cwd + "/" + specifier_it->second;
    Local<String> url_value = String::NewFromUtf8(isolate, str.c_str())
        .ToLocalChecked();
    meta->CreateDataProperty(context, url_key, url_value).ToChecked();
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
    char *content = read_file(absolute_path.c_str());
    if(content == NULL) {
        printf("Error: reading  module from %s\n", *path);
        return MaybeLocal<Module>();
    }
    Local<String> text = String::NewFromUtf8(isolate, content).ToLocalChecked();
    free(content);
    Local<PrimitiveArray> opts = PrimitiveArray::New(isolate, 2);
    opts->Set(isolate, 0, Integer::New(isolate, 0x5e0511));
    opts->Set(isolate, 1, specifier);
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
        opts
    );
    ScriptCompiler::Source source(text, origin);
    Local<Module> module;
    if(ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module)) {
        module_specifier_map.insert(std::make_pair(
            module->GetIdentityHash(), absolute_path));
        return module;
    }
    return MaybeLocal<Module>();
}

} // namespace

int main(int argc, char **argv) {
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    #ifdef V8_ENABLE_SANDBOX
    if(!v8::V8::InitializeSandbox()) {
        goto error;
    }
    #endif
    v8::V8::Initialize();
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate *isolate = Isolate::New(create_params);
    isolate->SetHostInitializeImportMetaObjectCallback(InitImportMetaObjectCallback);
    { // isolate_scope
    
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate, NULL, ObjectTemplate::New(isolate));
    Context::Scope context_scope(context);
    // context->SetAlignedPointerInEmbedderData(0, loop);
    Local<Object> g_obj = context->Global();
    
    const char *file_path = "/home/share/ryze/js/main.js";
    Local<String> str = String::NewFromUtf8(isolate, file_path).ToLocalChecked();
    Local<PrimitiveArray> opts = PrimitiveArray::New(isolate, 2);
    opts->Set(isolate, 0, Integer::New(isolate, 0x5e0619));
    opts->Set(isolate, 1, str);
    ScriptOrigin origin(
        isolate,
        str,
        0,
        0,
        true,
        -1,
        Local<Value>(),
        false,
        false,
        true,
        opts
    );
    char *file_content = read_file(file_path);
    Local<String> text = String::NewFromUtf8(isolate, file_content).ToLocalChecked();
    free(file_content);
    ScriptCompiler::Source source(text, origin);
    Local<Module> module;
    if(ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module)) {
        module_specifier_map.insert(std::make_pair(module->GetIdentityHash(), file_path));
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
    
    
    } // isolate_scope
    
error:
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}
