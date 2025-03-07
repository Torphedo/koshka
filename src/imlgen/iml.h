#pragma once
/// Less architecture-dependent representation of ARM code
/// (aka intermediate language)
///
/// Takes heavy inspiration from the Cemu PPC Recompiler:
/// https://github.com/cemu-project/Cemu/tree/main/src/Cafe/HW/Espresso/Recompiler
/// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompiler.h
/// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompilerIml.h
///
/// For the moment we're trying to split instructions into the simplest
/// possible operations, so 1 ARM instruction might create multiple IML
/// instructions (e.g. add immediate with negate would be an immediate add,
/// then a negate). Some instructions modify an immediate value before use to
/// save space, for the moment we'll just do that ahead of time in this stage.

#include <common/int.h>
#include <common/list.h>
#include <pool.h>
#include "arm_encoding.h"

typedef enum {
    DATA_OP_ADD,
    DATA_OP_SUB,
    DATA_OP_MUL,
    DATA_OP_DIV,

    // Multiply & add/sub
    DATA_OP_MUL_ADD,
    DATA_OP_MUL_SUB,

    // Logical left/right shift
    DATA_OP_LSL,
    DATA_OP_LSR,

    // Arithmetic left/right shift
    DATA_OP_ASL,
    DATA_OP_ASR,

    // Rotate left/right
    DATA_OP_ROL,
    DATA_OP_ROR,

    // Bitwise operations
    DATA_OP_AND,
    DATA_OP_OR,
    DATA_OP_XOR,
    DATA_OP_NEG, // Negate

    // Different variants of moving data between registers
    DATA_OP_MOVZ,
    DATA_OP_MOVK,
    // No need for MOVN, we'll handle that during decoding


    DATA_OP_BITFIELD_MOV,
    DATA_OP_BITFIELD_MOV_SIGNED,
    DATA_OP_BITFIELD_MOV_UNSIGNED,
}iml_math_op;

typedef enum {
    IML_OPERAND_IMMEDIATE,
    IML_OPERAND_REGISTER,
}iml_operand_type;

typedef struct {
    // Operand can be either a register or immediate value
    iml_operand_type type: 2;
    // The poor man's std::optional, set true if there's meaningful data in this entry
    bool exists: 1;
    union {
        u8 reg; // Register ID
        // The immediate can be as large as a 64-bit bitmask
        // TODO: See if we can separate the 32-bit and 64-bit variant to save
        // space on the IML tree
        u64 imm;
    };
}iml_operand;

// Specialized format for load/store instructions
typedef struct {

}iml_instr_load_store;

// Specialized format for instructions that operate on simple integer data
// (single destination, non-SIMD)
typedef struct {
    // Destinations in this type of instruction are always a register
    u8 dest_register;
    bool set_flags: 1; // Whether to set CPU state flags with operation result
    // If set, clear the rest of the register to 0 when doing a MOV operation.
    // Otherwise, leave them alone.
    bool mov_zero: 1;
    iml_math_op operation;
    iml_operand sources[3]; // Up to 3 operands, could be immediate or register
}iml_instr_data;

typedef struct {
    // Different variants of instruction
    L1_page variant;
    union {
        iml_instr_data data; // Data-processing instructions
        iml_instr_load_store load_store;
    };

    // Instructions can reference just the 32-bit portion of a 64-bit
    // register, or the whole thing
    bool is_64bit: 1;

    // Whether CPU state flags may be modified
    bool touched_negative_flag: 1;
    bool touched_zero_flag: 1;
    bool touched_carry_flag: 1;
    bool touched_overflow_flag: 1;

    // Address of next instruction
    pool_handle next;
}iml_instr;

// Poor man's std::optional<u32>. Used for a branch destination address.
typedef struct {
    u32 dest;
    bool exists;
}bdest;

// Tree of IML instructions
typedef struct {
    // We use a pool for the entire tree so it's easily destroyed, and branches
    // can reference their destination in a simple arch-independent way
    pool_t iml_pool;
    pool_t arm_pool; // Pool of ARM instructions
    pool_handle entry_point; // The first instruction
    // All known branch destinations that have already been decoded
    list branch_dests;
}iml_program;

iml_program imlgen(u8* arm_code, u32 size);
