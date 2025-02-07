#pragma once
/// Less architecture-dependent representation of ARM code
/// (aka intermediate /// language)
///
/// Takes heavy inspiration from the Cemu PPC Recompiler:
/// https://github.com/cemu-project/Cemu/tree/main/src/Cafe/HW/Espresso/Recompiler
/// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompiler.h
/// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompilerIml.h

#include <common/int.h>
#include "pool.h"
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
}iml_math_op;

typedef enum {
    IML_OPERAND_REGISTER,
    IML_OPERAND_IMMEDIATE,
}iml_operand_type;

typedef struct {
    iml_operand_type type: 2;
    union {
        struct {
            // 5 bits is enough for any "normal" ARM or x86 register number
            u8 id: 5;
            // Operation to apply to the register (usually shifting)
            iml_math_op operation;
            // Amount of shift/rotate/etc. to apply. Set to 0 if N/A
            u8 op_amount;
            bool negate; // Optionally negate the register
        }reg;
        u16 imm;
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
    iml_math_op operation;
    iml_operand sources[3];
}iml_instr_data;

typedef struct {
    // Different variants of instruction
    L1_page variant;
    union {
        iml_instr_data math;
        iml_instr_load_store load_store;
    };

    // Instructions can reference just the 32-bit portion of a 64-bit
    // register, or the whole thing
    bool is_64bit;
}iml_instr;

// Tree of IML instructions
typedef struct {
    // We use a pool for the entire tree so it's easily destroyed, and branches
    // can reference their destination in a simple arch-independent way
    pool_t instruction_pool;
    pool_handle entry_point; // The first instruction
}iml_program;
