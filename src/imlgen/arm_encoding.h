#ifndef A64_ENC_H
#define A64_ENC_H
/* Helper functions & enums for decoding ARMv8 instructions.
 * We basically turn the ARM encoding into enums that can be switched over.
 *
 * The Tegra X1 uses a Cortex A57 design, which implements ARMv8.0.
 * So, the ISA manual we reference is issue A.k of the ARMv8-A manual:
 *
 * https://developer.arm.com/documentation/ddi0487/ak/?lang=en
 *
 * The *_get_group functions 
*/

#include <stdbool.h>

#include <common/int.h>
// Instructions are divided into a 3-level tree of "instruction pages", which
// we use to quickly identify them.
// - Level 1: Very broad category (branch, load/store, etc.)
// - Level 2: Slightly more specific category (conditional branch, branch-to-register, etc.)
// - Level 3: Identifies the specific instruction
// Functions in this file narrow an instruction down to a level 2 page, but not
// down to the specific instruction level.

// Top-level instruction pages
// See C4.1 on pg. C4-192
typedef enum {
    L1_UNALLOCATED,
    L1_DATA_IMM,
    L1_BRANCH, // Also exception generating & system instructions
    L1_LD_STR, // Load/store
    L1_DATA_REG,
    L1_DATA_SIMD, // (0b1111 << 25) && (0b0111 << 25) are the same group
}L1_page;

// Returns the top-level instruction page the instruction belongs into.
// UNALLOCATED means the instruction is invalid.
L1_page instr_get_group(u32 instr);

// Categories of data processing instructions with immediate values.
// Use only with instructions categorized as DATA_IMM with instr_get_group().
// See C4.2 on pg. C4-193
typedef enum {
    L2_DATA_IMM_UNALLOCATED, // Invalid instruction
    L2_DATA_IMM_PC_REL_ADDR, // Load PC-relative address into a register
    L2_DATA_IMM_ADDSUB_IMM,  // Add/subtract immediate value
    L2_DATA_IMM_LOGICAL_IMM, // Bitwise operation between register & imm. data
    L2_DATA_IMM_MOV_WIDE,    // MOV 16-bit value to register
    L2_DATA_IMM_BITFIELD,    //
    L2_DATA_IMM_EXTRACT      //
}L2_data_imm_page;
L2_data_imm_page data_imm_get_group(u32 instr);

// Categories of branch instructions
// Use only with instructions categorized as L1_BRANCH with instr_get_group().
typedef enum {
    L2_BRANCH_CONDITIONAL,
    L2_BRANCH_EXCEPTION,
    L2_BRANCH_SYSTEM,
    L2_BRANCH_UNCONDITIONAL_REG,
    L2_BRANCH_UNCONDITIONAL_IMM,
    L2_BRANCH_COMPARE, // Compares an entire register
    L2_BRANCH_TEST, // Compares a single bit in a register
    L2_BRANCH_UNALLOCATED, // Compares a single bit in a register
}L2_branch_page;
L2_branch_page branch_get_group(u32 instr);

// Categories of load/store [memory operation] instructions.
// Use only with instructions categorized as LD_STR with instr_get_group().
// See C4.4 on pg. C4-202
typedef enum {
    L2_SIMD_MULTI_STRUCT,
    L2_SIMD_MULTI_STRUCT_POST_IDX,
    L2_SIMD_SINGLE_STRUCT,
    L2_SIMD_SINGLE_STRUCT_POST_IDX,

    L2_EXCLUSIVE,
    L2_REG_LITERAL,
    L2_REGPAIR_NO_ALLOC //
}L2_mem_page;
L2_mem_page ld_str_get_group(u32 instr);

// Categories of data processing on registers
// Use only with instructions categorized as DATA_REG with instr_get_group().
// See C4.5 on pg. C4-224
typedef enum {
    // Data processing w/ different numbers of sources
    L2_DATA_REG_3_SOURCES,
    L2_DATA_REG_2_SOURCES,
    L2_DATA_REG_1_SOURCE,

    L2_DATA_REG_LOGICAL_SHIFT,

    // Add/subtract
    L2_DATA_REG_ADDSUB_SHIFT,
    L2_DATA_REG_ADDSUB_EXTEND,
    L2_DATA_REG_ADDSUB_CARRY,

    // Conditional compare with register or immediate
    L2_DATA_REG_COND_COMP_REG,
    L2_DATA_REG_COND_COMP_IMM,

    // Conditional select
    L2_DATA_REG_COND_SEL,

    // Invalid instruction
    L2_DATA_REG_UNALLOCATED,
}L2_data_reg_page;
L2_data_reg_page data_reg_get_group(u32 instr);

#endif // #ifndef A64_ENC_H
