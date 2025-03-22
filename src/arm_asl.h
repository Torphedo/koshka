#pragma once
// This file has implementations for many of the ASL (Architecture
// Specification Language) pseudocode functions used throughout the spec.

#include <common/int.h>
#include <stdbool.h>

/// @brief Decode AArch64 bitfield and logical immediate masks which use a similar encoding structure
///
/// ARMv8 spec page J1-5282
/// https://kddnewton.com/2022/08/11/aarch64-bitmask-immediates.html
/// @param immN Whether the bit pattern is 64-bit (rather than 32-bit)
/// @param imms Encodes the size and number of sequential 1s
/// @param immr The number of right rotations to apply
/// @param immediate From spec, unused.
u64 DecodeBitMasks(bool immN, u8 imms, u8 immr, bool immediate);
