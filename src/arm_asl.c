#include "arm_asl.h"
#include "bitmanip.h"
#include <assert.h>
#include <common/logging.h>

// ARMv8 spec page J1-5402
s8 HighestBitSet(u64 x) {
    for (u8 i = NUM_BITS(x) - 1; i >= 0; i--) {
        if (GET_SINGLE_BIT(x, i)) {
            return i;
        }
    }
    return -1;
}

// Rotate right
// ARMv8 spec page J1-5404
u64 ROR(u64 x, u8 bit_size, u8 shift) {
    for (u8 i = 0; i < shift; i++) {
        x = (x << (bit_size - 1)) | (x >> 1);
    }
    return x;
}

u64 DecodeBitMasks(bool immN, u8 imms, u8 immr, bool immediate) {
    // Sorry about the terse variable names, this is taken the spec and I don't
    // fully understand how the encoding is meant to work. I couldn't get the
    // spec's code to work, so some of the math comes from LLVM:
    // https://llvm.org/doxygen/AArch64AddressingModes_8h_source.html#l00293
    // - Torph

    // These are supposed to 6-bit values, but we can't make that type.
    assert(imms <= MAX_VAL_FOR_SIZE(6));
    assert(immr <= MAX_VAL_FOR_SIZE(6));

    const s8 len = HighestBitSet((((u8)immN) << 6) | (((u8)~imms) & MAX_VAL_FOR_SIZE(6)));
    assert(len > 0);
    assert(len <= 7);

    u8 size = 1 << len;
    const u8 S = imms & (size - 1);
    const u8 R = immr & (size - 1);
    assert(S != size - 1);

    u64 pattern = (1 << (S + 1)) - 1;
    pattern = ROR(pattern, size, R);

    while (size != 64) {
        pattern |= (pattern << size);
        size *= 2;
    }

    LOG_MSG(debug, "Decoded bitmask value %d\n", pattern);
    return pattern;
}
