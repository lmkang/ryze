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
using v8::NewStringType;
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

MaybeLocal<String> ReadFile(Isolate *isolate, const char *name);
void Log(const FunctionCallbackInfo<Value> &args);
void ReportException(Isolate *isolate, TryCatch *try_catch);

std::map<std::pair<std::string, ModuleType>, Global<Module>> module_map;
std::unordered_map<Global<Module>, std::string, ModuleGlobalHash> module_to_specifier_map;


std::string GetWorkingDirectory() {
#if defined(_WIN32) || defined(_WIN64)
    char system_buffer[MAX_PATH];
    // Unicode paths are unsupported, which is fine as long as
    // the test directory doesn't include any such paths.
    DWORD len = GetCurrentDirectoryA(MAX_PATH, system_buffer);
    return system_buffer;
#else
    char curdir[PATH_MAX];
    getcwd(curdir, PATH_MAX);
    return curdir;
#endif
}

bool IsAbsolutePath(const std::string &path) {
#if defined(_WIN32) || defined(_WIN64)
    // This is an incorrect approximation, 
    //but should work for all our test-running cases.
    return path.find(':') != std::string::npos;
#else
    return path[0] == '/';
#endif
}

std::string NormalizePath(const std::string &path, const std::string &dir_name) {
    std::string absolute_path;
    if(IsAbsolutePath(path)) {
        absolute_path = path;
    } else {
        absolute_path = dir_name + '/' + path;
    }
    std::replace(absolute_path.begin(), absolute_path.end(), '\\', '/');
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

MaybeLocal<Module> ResolveModuleCallback(Local<Context> context,
    Local<String> specifier,
    Local<FixedArray> import_assertions,
    Local<Module> referrer) {
    Isolate *isolate = context->GetIsolate();
    std::shared_ptr<ModuleEmbedderData> module_data =
        GetModuleDataFromContext(context);
    auto specifier_it = module_data->module_to_specifier_map.find(
        Global<Module>(isolate, referrer));
    assert(specifier_it != module_data->module_to_specifier_map.end());
    std::string absolute_path = NormalizePath(
        ToSTLString(isolate, specifier), DirName(specifier_it->second));
    ModuleType module_type = ModuleEmbedderData::ModuleTypeFromImportAssertions(
        context, import_assertions, true);
    auto module_it =
        module_data->module_map.find(std::make_pair(absolute_path, module_type));
    assert(module_it != module_data->module_map.end());
    return module_it->second.Get(isolate);
}

bool ExecuteModule(Isolate *isolate, const char *file_name) {
    HandleScope handle_scope(isolate);
    Local<Context> realm(isolate->GetCurrentContext());
    Context::Scope context_scope(realm);
    std::string absolute_path = NormalizePath(file_name, GetWorkingDirectory());
    // Use a non-verbose TryCatch and report exceptions manually using
    // Shell::ReportException, because some errors (such as file errors) are
    // thrown without entering JS and thus do not trigger
    // isolate->ReportPendingMessages().
    TryCatch try_catch(isolate);
    std::shared_ptr<ModuleEmbedderData> module_data =
        GetModuleDataFromContext(realm);
    Local<Module> root_module;
    auto module_it = module_data->module_map.find(
        std::make_pair(absolute_path, ModuleType::kJavaScript));
    if(module_it != module_data->module_map.end()) {
        root_module = module_it->second.Get(isolate);
    } else if(!FetchModuleTree(Local<Module>(), realm, absolute_path,
        ModuleType::kJavaScript).ToLocal(&root_module)) {
        assert(try_catch.HasCaught());
        ReportException(isolate, &try_catch);
        return false;
    }
    module_data->origin = absolute_path;
    MaybeLocal<Value> maybe_result;
    if(root_module->InstantiateModule(realm, ResolveModuleCallback).FromMaybe(false)) {
        maybe_result = root_module->Evaluate(realm);
        assert(!maybe_result.IsEmpty());
        EmptyMessageQueues(isolate);
    }
    Local<Value> result;
    if(!maybe_result.ToLocal(&result)) {
        assert(try_catch.HasCaught());
        ReportException(isolate, &try_catch);
        return false;
    }
    // Loop until module execution finishes
    Local<Promise> result_promise(result.As<Promise>());
    while(result_promise->State() == Promise::kPending 
        && reinterpret_cast<i::Isolate*>(isolate)
            ->default_microtask_queue()
            ->size() > 0) {
        Shell::CompleteMessageLoop(isolate);
    }
    if(result_promise->State() == Promise::kRejected) {
        // If the exception has been caught by the promise pipeline, we rethrow
        // here in order to ReportException.
        // TODO(cbruni): Clean this up after we create a new API for the case
        // where TLA is enabled.
        if(!try_catch.HasCaught()) {
            isolate->ThrowException(result_promise->Result());
        } else {
            DCHECK_EQ(try_catch.Exception(), result_promise->Result());
        }
        ReportException(isolate, &try_catch);
        return false;
    }
    std::vector<std::tuple<Local<Module>, Local<Message>>> stalled =
        root_module->GetStalledTopLevelAwaitMessage(isolate);
    if(stalled.size() > 0) {
        Local<Message> message = std::get<1>(stalled[0]);
        ReportException(isolate, message, v8::Exception::Error(message->Get()));
        return false;
    }
    assert(!try_catch.HasCaught());
    return true;
}

MaybeLocal<String> ReadFile(Isolate *isolate, const char *name) {
    FILE *file = fopen(name, "rb");
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








































