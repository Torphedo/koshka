#pragma once

#include <stdbool.h>
#include <common/int.h>

bool file_is_elf(const char* path);
u8* load_elf(const char* path);
