#ifndef EMIT_H
#define EMIT_H

#include "types.h"
#include "struct/vfile.h"

void emit(vfile* file, u32 instr);
void buf_translate(vfile* src, vfile* dest);

#endif // #ifndef EMIT_H

