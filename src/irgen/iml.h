#pragma once

#include <common/int.h>

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
        // 5 bits is enough for any "normal" ARM or x86 register number
        u8 reg: 5;
        u16 imm;
    };
}iml_operand;

typedef struct {
    // TODO: Break into more hierarchical operation types to reduce switch cases

    // Categorizes instructions into the broad categories from the ARM spec
    iml_op operation;

    // More specific categories
    union {
        iml_branch_type branch_type;
    };

    iml_operand op1;
    iml_operand op2;
    iml_operand op3;
}iml_instr;
