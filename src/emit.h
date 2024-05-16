#ifndef EMIT_H
#define EMIT_H

#include "common/vfile.h"

void buf_translate(vfile* src, vfile* dest);
vfile translate_file(const char* path);

#endif // #ifndef EMIT_H

