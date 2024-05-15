#include <string.h>
#include <stdbool.h>

#include "types.h"
#include "regalloc.h"

#define X86_GP_COUNT (14)

// This is a naive implementation that doesn't bother to account for lifetimes.
// We could temporarily spill to stack and re-use some registers in long
// functions where a register is only used in some sections.

// Array of ARM GPR IDs (+1 so that the uninitialized zeroes don't look like
// everything is claimbed by X0).
// Making these static means they'll have one instance per thread, allowing for
// multi-threaded translation (eventually). RSP & RBP are specially reserved for
// their ARM counterparts.

static u8 taken_gp[X86_GP_COUNT + 2] = {[RSP] = 0xFF, [RBP] = 0xFF};
static bool full = false;

s8 reg_alloc(u8 arm_reg_id) {
    // Early exit, should be useful for large functions when we're out of GPRs.
    if (full) {
        return -1;
    }
    // Array inits to zero, we add one so it doesn't look claimed by r0
    arm_reg_id++;

    for (u8 i = 0; i < X86_GP_COUNT; i++) {
        if (taken_gp[i] == arm_reg_id) {
            // This ARM register is already mapped to gp[i].
            return i;
        }
        else if (taken_gp[i] == 0) {
            // Register is unused. Claim it & return the x86 register ID!
            taken_gp[i] = arm_reg_id;
            return i;
        }
    }

    // Special cases. We subtract 1 to account for the increment earlier
    // X29 is the frame pointer
    if ((arm_reg_id - 1) == 29) {
        taken_gp[RBP] = arm_reg_id;
    }
    // There is no X31, but SP is encoded in instructions as ID 31.
    else if ((arm_reg_id - 1) == 31) {
        taken_gp[RSP] = arm_reg_id;
    }

    // Oh well... resort to the stack, we're all out.
    full = true;
    return -1;
}

// Reset all register mappings to 0.
void reg_reset() {
    memset(&taken_gp, 0x00, sizeof(taken_gp));
    full = false;
}

