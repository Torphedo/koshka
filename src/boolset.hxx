#pragma once
// Basically std::bitset, but able to set more than 1 bit at a time
#include <common/int.h>

struct boolset {

    // TODO: For sets under <= 64 bits we can use a union to back this with a
    // single u64 instead of a heap buffer
    u8* buf = nullptr;
    u32 byte_size = 0;
    bool initialized = false;

    boolset(u32 bit_count);
    ~boolset();

    bool get_bit(u32 pos);
    void set_bit(u32 pos, bool value);

    void set_range(u32 pos, u32 size, bool value);

    void set_all(bool value);
};
