#ifndef RYZE_ESM_H
#define RYZE_ESM_H

#include "common.h"

void import_meta_obj_callback(
        Local<Context> context,
        Local<Module> module,
        Local<Object> meta);
int esm_init(Isolate *isolate, const char *file_path);
void esm_destroy();

#endif
