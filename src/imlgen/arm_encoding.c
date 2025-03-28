#include "arm_encoding.h"
#include "bitmanip.h"

// TODO: Make this file C++ and all the functions constexpr so they can be
// tested via static_assert().

// A lookup table entry used for instruction decoding.
// The ARM spec uses a compact bitmask notation where 'x' means the bit's value
// doesn't matter. We have to use 2 bitmasks to recreate that behaviour.
typedef struct {
    // Instruction will be ANDed with this mask
    u32 mask;
    // Then it will be compared with this pattern
    u32 pattern;
    // If it matches the pattern, the instruction belongs to this group.
    u32 group;
}arm_page_entry;

// Lookup table entry to check 2 values against 2 bitmasks.
typedef struct {
    // First value will be ANDed with this mask and compared with the pattern
    u32 mask1;
    u32 pattern1;

    // Same but for the second value
    u32 mask2;
    u32 pattern2;

    // If it matches both patterns, the instruction belongs to this group.
    u32 group;
}arm_double_page_entry;

// See pg. C4-192. This lookup table encodes the bit patterns to compare op0
// against, and the resulting instruction page if they match.
static const arm_page_entry top_instr_pagetable[] = {
    {0b1110, 0b1000, L1_DATA_IMM},
    {0b1110, 0b1010, L1_BRANCH},
    {0b0101, 0b0100, L1_LD_STR},
    {0b0111, 0b0101, L1_DATA_REG},

    // For some reason these cases both indicate the same group.
    {0b1111, 0b0111, L1_DATA_SIMD},
    {0b1111, 0b1111, L1_DATA_SIMD},
};

L1_page instr_get_group(u32 instr) {
    const u8 op0 = GET_BIT_REGION(instr, 25, 29);

    for (u32 i = 0; i < ARRAY_SIZE(top_instr_pagetable); i++) {
        const arm_page_entry entry = top_instr_pagetable[i];
        if ((op0 & entry.mask) == entry.pattern) {
            return entry.group;
        }
    }

    // There was no matching pattern, instruction is invalid.
    return L1_UNALLOCATED;
}

// See C4.2 on pg. C4-193
static const arm_page_entry data_imm_pagetable[] = {
    {0b110, 0b000, L2_DATA_IMM_PC_REL_ADDR},
    {0b110, 0b010, L2_DATA_IMM_ADDSUB_IMM},
    {0b111, 0b100, L2_DATA_IMM_LOGICAL_IMM},
    {0b111, 0b101, L2_DATA_IMM_MOV_WIDE},
    {0b111, 0b110, L2_DATA_IMM_BITFIELD},
    {0b111, 0b111, L2_DATA_IMM_EXTRACT},
};

L2_data_imm_page data_imm_get_group(u32 instr) {
    const u8 op0 = GET_BIT_REGION(instr, 23, 25);

    for (u32 i = 0; i < ARRAY_SIZE(data_imm_pagetable); i++) {
        const arm_page_entry entry = data_imm_pagetable[i];
        if ((op0 & entry.mask) == entry.pattern) {
            return entry.group;
        }
    }

    // No matching pattern, instruction is invalid.
    return L2_DATA_IMM_UNALLOCATED;
}

// See section C4.3 on pg. C4-197 for the table where all these values come from.
static const arm_double_page_entry branch_pagetable[] = {
    {0b111, 0b010, 0b1000, 0b0000, L2_BRANCH_CONDITIONAL},

    {0b111, 0b110, 0b1100, 0b0000, L2_BRANCH_EXCEPTION},
    {0b111, 0b110, 0b1111, 0b0100, L2_BRANCH_SYSTEM},

    {0b111, 0b110, 0b1000, 0b1000, L2_BRANCH_UNCONDITIONAL_REG},

    {0b011, 0b000, 0b0000, 0b0000, L2_BRANCH_UNCONDITIONAL_IMM},
    {0b011, 0b001, 0b1000, 0b0000, L2_BRANCH_COMPARE},
    {0b011, 0b001, 0b1000, 0b1000, L2_BRANCH_TEST},
};

L2_branch_page branch_get_group(u32 instr) {
    const u8 op0 = GET_BIT_REGION(instr, 29, 31);
    const u8 op1 = GET_BIT_REGION(instr, 22, 25);

    for (u32 i = 0; i < ARRAY_SIZE(branch_pagetable); i++) {
        const arm_double_page_entry entry = branch_pagetable[i];
        if ((op0 & entry.mask1) == entry.pattern1) {
            if ((op1 & entry.mask2) == entry.pattern2) {
                return entry.group;
            }
        }
    }

    // There was no matching pattern, instruction is invalid.
    return L2_BRANCH_UNALLOCATED;
}

L2_data_reg_page data_reg_get_group(u32 instr) {
    const u8 op0 = GET_SINGLE_BIT(instr, 30);
    const u8 op1 = GET_SINGLE_BIT(instr, 28);
    const u8 op2 = GET_BIT_REGION(instr, 21, 24);
    const u8 op3 = GET_SINGLE_BIT(instr, 11);

    // Sorry for all the magic numbers, there's not much I can do to make it
    // more intuitive. See C-4.5 on page C4-224 for the original table.
    if (op1 == 0) {
        if (GET_SINGLE_BIT(op2, 3) == 0) {
            return L2_DATA_REG_LOGICAL_SHIFT;
        }

        if ((op2 & 0b1001) == 0b1000) {
            return L2_DATA_REG_ADDSUB_SHIFT;
        }

        if ((op2 & 0b1001) == 0b1001) {
            return L2_DATA_REG_ADDSUB_EXTEND;
        }
    }

    if (op1 == 1) {
        if (op2 == 0b0000) {
            return L2_DATA_REG_ADDSUB_CARRY;
        }

        const bool conditional_compare = (op2 == 0b0010);
        if (conditional_compare) {
            if (op3) {
                return L2_DATA_REG_COND_COMP_IMM;
            } else {
                return L2_DATA_REG_COND_COMP_REG;
            }
        }

        if (op2 == 0b0100) {
            return L2_DATA_REG_COND_SEL;
        }

        if (op2 == 0b0110) {
            if (op0 == 0) {
                return L2_DATA_REG_2_SOURCES;
            } else {
                return L2_DATA_REG_1_SOURCE;
            }
        }

        if ((op2 & 0b1000) == 0b1000) {
            return L2_DATA_REG_3_SOURCES;
        }
    }

    // No pattern match, instruction is invalid.
    return L2_DATA_REG_UNALLOCATED;
}
