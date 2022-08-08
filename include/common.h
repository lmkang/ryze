#ifndef CATTLE_COMMON_H_
#define CATTLE_COMMON_H_

#include "v8/v8.h"
#include "v8/libplatform/libplatform.h"

namespace cattle {

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

} // namespace cattle

#endif