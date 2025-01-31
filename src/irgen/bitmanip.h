#ifndef BITMANIP_H
#define BITMANIP_H

#define NUM_BITS(val) (sizeof(val) * 8)

// Get a value the same size as the original, filled with all 1s
#define SET_ALL(val) (~((val) ^ (val)))

#define GET_SINGLE_BIT(val, bit) (((val) >> (bit)) & 1)

// Mask out a region of bits.
// The first half discards the lower bits, and the second half discards the higher bits.
#define GET_BIT_REGION(val, bit_low, bit_high) (((val) >> (bit_low)) & (SET_ALL(val) >> (NUM_BITS(val) - bit_high)))

#endif // #ifndef BITMANIP_H
