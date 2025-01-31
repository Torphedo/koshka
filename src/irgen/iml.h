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

typedef enum {
    IML_OP_ADD,
    IML_OP_ADD_CARRY,
    IML_OP_BRANCH,
}iml_op;

typedef enum {
    IML_BRANCH_UNCONDITIONAL,
    IML_BRANCH_CONDITIONAL,
}iml_branch_type;

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
            // Instructions can reference just the 32-bit portion of a 64-bit
            // register, or the whole thing
            bool is_32bit: 1;
        }reg;
        u16 imm;
    };
}iml_operand;

typedef struct {
    // TODO: Break into more hierarchical operation types to reduce switch cases

    // Categorizes instructions into the broad categories from the ARM spec
    iml_op operation;

    // More specific categories
    union {
        // TODO: Maybe replace with flags instead of an enum if needed
        struct {
            iml_branch_type type;
            // Destination instruction, or POOL_INVALID_VALUE for
            // branch-to-register.
            pool_handle dest;
        }branch_info;
    };

    iml_operand op1;
    iml_operand op2;
    iml_operand op3;
}iml_instr;

// Tree of IML instructions
typedef struct {
    // We use a pool for the entire tree so it's easily destroyed, and branches
    // can reference their destination in a simple arch-independent way
    pool_t instruction_pool;
    pool_handle entry_point; // The first instruction
}iml_program;
