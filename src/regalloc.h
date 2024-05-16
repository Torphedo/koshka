#ifndef REGALLOC_H
#define REGALLOC_H

#include "common/types.h"

// All 14 available general-purpose x86 registers.
// Ordering matches the (weird) x86 register encoding:
// https://www.cs.uaf.edu/2016/fall/cs301/lecture/09_28_machinecode.html
typedef enum {
    RAX,
    RCX,
    RDX,
    RBX,
    RSP, // Mapped to SP/R31
    RBP, // Mapped to X29
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
}x86_reg;

// These functions have shared state per thread.

// Find an x86 register to map an ARM register to. Returns x86_reg, or -1 when
// there are no open registers. If -1, you should spill to stack.
s8 reg_alloc(u8 arm_reg_id);

// Reset all stored register mappings to 0.
void reg_reset();

#endif // #ifndef REGALLOC_H

