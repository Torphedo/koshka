#ifndef A64_ENC_H
#define A64_ENC_H
/* Helper functions & enums for decoding ARMv8 instructions.
 *
 * The Tegra X1 uses a Cortex A57 design, which implements ARMv8.0.
 * So, the ISA manual used here is issue A.k of the ARMv8-A manual:
 *
 * Page numbers referenced are for this manual:
 * https://developer.arm.com/documentation/ddi0487/ak/?lang=en
 *
 * The *_get_group functions 
*/

#include <stdbool.h>

#include <common/int.h>

// Encodings for broad categories of instructions
// See C4.1 on pg. C4-192
typedef enum {
    UNALLOCATED,
    DATA_IMM,
    BRANCH, // Also exception generating & system instructions
    LD_STR,
    DATA_REG,
    DATA_SIMD, // (0b1111 << 25) && (0b0111 << 25) are the same group
}enc_cat;

// Returns the instruction group the instruction fits into. UNALLOCATED means
// the instruction is invalid.
enc_cat instr_get_group(u32 instr);

// Categories of data processing instructions with immediate values.
// Use only with instructions categorized as DATA_IMM with instr_get_group().
// See C4.2 on pg. C4-193
typedef enum {
    pc_rel_adr,  // Load PC-relative address into a register
    addsub_imm,  // Add/subtract immediate value
    logical_imm, // Bitwise operation between register & imm. data
    mov_wide,    // MOV 16-bit value to register
    bitfield,    // 
    extract      //
}data_imm_cat;
data_imm_cat data_imm_get_group(u32 instr);

// Categories of load/store instructions.
// Use only with instructions categorized as LD_STR with instr_get_group().
// See C4.4 on pg. C4-202
typedef enum {
    SIMD_MULTI_STRUCT,
    SIMD_MULTI_STRUCT_POST_IDX,
    SIMD_SINGLE_STRUCT,
    SIMD_SINGLE_STRUCT_POST_IDX,

    EXCLUSIVE,
    REG_LITERAL,
    REGPAIR_NO_ALLOC // 
}ld_str_cat;
ld_str_cat ld_str_get_group(u32 instr);

// Categories of data processing on registers
// Use only with instructions categorized as DATA_REG with instr_get_group().
// See C4.5 on pg. C4-224
typedef enum {
    // Data processing from a different number of sources
    DATA_REG_3_SOURCES,
    DATA_REG_2_SOURCES,
    DATA_REG_1_SOURCE,

    DATA_REG_LOGICAL_SHIFT,

    DATA_REG_ADDSUB_SHIFT,
    DATA_REG_ADDSUB_EXTEND,
    DATA_REG_ADDSUB_CARRY,

    // Conditional compare with register or immediate
    DATA_REG_COND_COMP_REG,
    DATA_REG_COND_COMP_IMM,

    // Conditional select
    DATA_REG_COND_SEL,

    // Invalid instruction
    DATA_REG_UNALLOCATED,
}data_reg_cat;
data_reg_cat data_reg_get_group(u32 instr);

#endif // #ifndef A64_ENC_H
