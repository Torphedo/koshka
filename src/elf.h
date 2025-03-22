#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <common/int.h>

bool file_is_elf(const char* path);
u8* load_elf(const char* path, s64* size_out);

#ifdef __cplusplus
}
#endif
