#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iterator>
#include <unordered_map>
#include "esm.h"

extern "C" {
    #include "util.h"
}

namespace {

std::unordered_map<int, char *> module_path_map;

MaybeLocal<Module> resolve_module_callback(
        Local<Context> context,
        Local<String> specifier,
        Local<FixedArray> import_assertions,
        Local<Module> referer) {
    Isolate *isolate = context->GetIsolate();
    HandleScope handle_scope(isolate);
    String::Utf8Value value(isolate, specifier);
    auto it = module_path_map.find(referer->GetIdentityHash());
	char *dir = dirname(it->second);
    char *path = normalize_path(*value, dir);
	free(dir);
	char *content = NULL;
	if(is_http_path(path)) {
		content = http_get(path);
	} else {
		content = read_file(path);
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
        module_path_map.insert(
            std::make_pair(module->GetIdentityHash(), path));
        return module;
    }
    return MaybeLocal<Module>();
}

} // namespace

void import_meta_obj_callback(
        Local<Context> context,
        Local<Module> module,
        Local<Object> meta) {
    Isolate *isolate = context->GetIsolate();
    HandleScope handle_scope(isolate);
    auto it = module_path_map.find(module->GetIdentityHash());
    Local<String> key = String::NewFromUtf8Literal(isolate, "url", 
        NewStringType::kInternalized);
    Local<String> value = String::NewFromUtf8(isolate, it->second)
        .ToLocalChecked();
    meta->CreateDataProperty(context, key, value).ToChecked();
}

int esm_init(Isolate *isolate, const char *file_path) {
    HandleScope handle_scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    char *path = NULL;
    if(is_absolute_path(file_path)) {
        size_t len = strlen(file_path);
        path = (char *) malloc(len + 1);
        memcpy(path, file_path, len);
        path[len] = '\0';
    } else {
        char *p = getcwd(NULL, 0);
        path = normalize_path(file_path, p);
        free(p);
    }
    Local<String> str = String::NewFromUtf8(isolate, path).ToLocalChecked();
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
    char *content = NULL;
	if(is_http_path(path)) {
		content = http_get(path);
	} else {
		content = read_file(path);
	}
    Local<String> text = String::NewFromUtf8(isolate, content).ToLocalChecked();
    free(content);
    ScriptCompiler::Source source(text, origin);
    Local<Module> module;
    if(ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module)) {
		module_path_map.insert(std::make_pair(module->GetIdentityHash(), path));
		if(module->InstantiateModule(context, 
				resolve_module_callback).FromMaybe(false)) {
			Local<Value> result;
			if(module->Evaluate(context).ToLocal(&result)) {
				Local<Promise> promise(result.As<Promise>());
				while(promise->State() == Promise::kPending) {
					// Loop until module execution finish
				}
				return 1;
			}
		}
    }
	free(path);
	return 0;
}

void esm_destroy() {
	auto it = module_path_map.begin();
	while(it != module_path_map.end()) {
		free(it->second);
		module_path_map.erase(it++);
	}
}
