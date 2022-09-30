#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "console.h"
#include "esm.h"

int main(int argc, char **argv) {
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate *isolate = Isolate::New(create_params);
    isolate->SetHostInitializeImportMetaObjectCallback(import_meta_obj_callback);
    { // isolate_scope
    
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate, NULL, ObjectTemplate::New(isolate));
    Context::Scope context_scope(context);
    // context->SetAlignedPointerInEmbedderData(0, loop);
    Local<Object> g_obj = context->Global();
    console_init(isolate, g_obj);
    
    do {
        // init es module
        if(argc < 2) {
            printf("Error: can not find entry file\n");
            break;
        }
        if(!esm_init(isolate, argv[1])) {
            printf("Error: fail to init es module\n");
            break;
        }
        // printf("V8 version: %s\n", v8::V8::GetVersion());
    } while(0);
    
    } // isolate_scope
    esm_destroy();
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}
