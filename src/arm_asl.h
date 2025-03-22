#pragma once
// This file has implementations for many of the ASL (Architecture
// Specification Language) pseudocode functions used throughout the spec.

#include <common/int.h>
#include <stdbool.h>

// Decode AArch64 bitfield and logical immediate masks which use a similar encoding structure
// ARMv8 spec page J1-5282
u64 DecodeBitMasks(bool immN, u8 imms, u8 immr, bool immediate);
