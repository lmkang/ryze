#ifndef RYZE_COMMON_H
#define RYZE_COMMON_H

#include "v8.h"
#include "libplatform/libplatform.h"

#define V8_SET_OBJ(isolate, obj, name, obj2) \
    (obj)->Set((isolate)->GetCurrentContext(), \
        String::NewFromUtf8Literal(isolate, name, \
        NewStringType::kInternalized), obj2).ToChecked()

#define V8_SET_FUNC(isolate, obj, name, func) \
    (obj)->Set(String::NewFromUtf8Literal(isolate, name, \
        NewStringType::kInternalized), \
        FunctionTemplate::New(isolate, func))

#define V8_SET_VALUE(isolate, obj, name, value) \
    (obj)->Set(String::NewFromUtf8Literal(isolate, name, \
        NewStringType::kInternalized), value)

using v8::Isolate;
using v8::Context;
using v8::Platform;
using v8::Local;
using v8::Value;
using v8::Integer;
using v8::Uint32Array;
using v8::Float64Array;
using v8::String;
using v8::NewStringType;
using v8::ObjectTemplate;
using v8::FunctionTemplate;
using v8::FunctionCallbackInfo;
using v8::FunctionCallback;
using v8::Function;
using v8::Object;
using v8::PrimitiveArray;
using v8::Promise;
using v8::MaybeLocal;
using v8::Maybe;
using v8::Module;
using v8::Script;
using v8::ScriptCompiler;
using v8::ScriptOrigin;
using v8::ArrayBuffer;
using v8::SharedArrayBuffer;
using v8::BackingStore;
using v8::Exception;
using v8::TryCatch;
using v8::Message;
using v8::HandleScope;
using v8::FixedArray;
using v8::Global;

#endif
