#pragma once
#include <assert.h>
#include <common/int.h>

/// @brief Gets the maximum size of an arbitrarily sized integer (up to u64)
///
/// e.g. MAX_VAL_FOR_SIZE(6) = 0b111111, MAX_VAL_FOR_SIZE(24) = 0xFFFFFF, etc.
/// @param bits The size in bits
#define MAX_VAL_FOR_SIZE(bits) (~((u64)0) >> (64 - (bits)))

#define NUM_BITS(val) (sizeof(val) * 8)

// Get a value the same size as the original, filled with all 1s
#define SET_ALL(val) (~((val) ^ (val)))

#define GET_SINGLE_BIT(val, bit) (((val) >> (bit)) & 1)

#define HAS_BIT_FLAG(val, flag) (((val) & (flag)) != 0)

// Mask out a region of bits.
// The first half discards the lower bits, and the second half discards the higher bits.
#define GET_BIT_REGION(val, bit_low, bit_high) (((val) & (SET_ALL(val) >> (NUM_BITS(val) - bit_high - 1))) >> (bit_low))

static_assert(GET_SINGLE_BIT((u32)0, 5) == 0, "Broken bit extraction!");
static_assert(GET_SINGLE_BIT((u32)0b1010110, 1) == 1, "Broken bit extraction!");
static_assert(GET_BIT_REGION((u8)0b0010100, 2, 4) == 0b101, "Broken bit masking!");
static_assert(GET_BIT_REGION((u32)0xD2800020, 29, 30) == 0b10, "Broken bit masking!");
