#ifndef BITMANIP_H
#define BITMANIP_H
#include <assert.h>
#include <common/int.h>

#define NUM_BITS(val) (sizeof(val) * 8)

// Get a value the same size as the original, filled with all 1s
#define SET_ALL(val) (~((val) ^ (val)))

#define GET_SINGLE_BIT(val, bit) (((val) >> (bit)) & 1)

// Mask out a region of bits.
// The first half discards the lower bits, and the second half discards the higher bits.
#define GET_BIT_REGION(val, bit_low, bit_high) (((val) & (SET_ALL(val) >> (NUM_BITS(val) - bit_high - 1))) >> (bit_low))

static_assert(GET_BIT_REGION((u8)0b0010100, 2, 4) == 0b101, "Broken bit masking!");
static_assert(GET_BIT_REGION((u32)0xD2800020, 29, 30) == 0b10, "Broken bit masking!");

#endif // #ifndef BITMANIP_H
