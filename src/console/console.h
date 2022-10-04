#ifndef RYZE_CONSOLE_H
#define RYZE_CONSOLE_H

#include "common.h"

void console_init(Isolate *isolate);
void console_log(const FunctionCallbackInfo<Value> &args);

#endif
