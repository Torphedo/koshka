#pragma once

/// Less architecture-dependent representation of ARM code
/// (aka intermediate language)
///
/// Takes heavy inspiration from the Cemu PPC Recompiler:
/// https://github.com/cemu-project/Cemu/tree/main/src/Cafe/HW/Espresso/Recompiler
/// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompiler.h
/// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompilerIml.h
///
/// We split instructions into a series of simple micro-operations like a real
/// CPU would. This lets us re-use code to implement specialized instructions.
/// (e.g. add immediate with negate would be an immediate add, then a negate).

#include <common/int.h>
#include <common/list.h>
#include <pool.h>
#include "arm_encoding.h"

namespace iml {

// All potential math operations (non-SIMD)
enum math_op : u8 {
    MATH_OP_ADD,
    MATH_OP_SUB,
    MATH_OP_MUL,
    MATH_OP_DIV,

    // Multiply & add/sub
    MATH_OP_MUL_ADD,
    MATH_OP_MUL_SUB,

    // Logical left/right shift
    MATH_OP_LSL,
    MATH_OP_LSR,

    // Arithmetic left/right shift
    MATH_OP_ASL,
    MATH_OP_ASR,

    // Rotate left/right
    MATH_OP_ROL,
    MATH_OP_ROR,

    // Bitwise operations
    MATH_OP_AND,
    MATH_OP_OR,
    MATH_OP_XOR,
    MATH_OP_NEG, // Negate

    MATH_OP_BITFIELD_MOV,
    MATH_OP_BITFIELD_MOV_SIGNED,
    MATH_OP_BITFIELD_MOV_UNSIGNED,

    MATH_OP_ENUMMAX,
};

static const math_op shift_type_table[] = {
    MATH_OP_LSL,
    MATH_OP_LSR,
    MATH_OP_ASR,
    MATH_OP_ROR,
};

static const math_op bitwise_op_table[] = {
    MATH_OP_AND,
    MATH_OP_OR,
    MATH_OP_XOR,
    MATH_OP_AND,
};

enum operand_type : u8 {
    OPERAND_IMMEDIATE,
    OPERAND_REGISTER,
};

// Many instructions will operate on a register value before using it in
// another operation, but don't mutate the register itself.
// For example BIC (bitwise clear) can encode things like this:
//                    Rd = Rn & (~(Rm >> 7))
//  ... but only Rd is mutated.
// To handle this, we build an expression tree.

// A single operand to an expression
struct operand {
    // Operand can be either a register or immediate value
    operand_type type: 2;
    union {
        u8 reg; // Register ID
        // The immediate can be as large as a 64-bit bitmask
        // TODO: See if we can separate the 32-bit and 64-bit variant to save
        // space on the IML tree
        u64 imm;
    };
};

// Essentially a typed pool handle to an iml_expression.
typedef pool_handle expression_handle;

// Recursive expression tree type.
struct expression {
    // Union tag for the expression. Indicates if you should interpret it as a
    // final value (a register or immediate) or another layer of expression.
    bool is_value;
    const union {
        struct {
            math_op op;

            // The left and right sub-expressions
            expression_handle left;
            expression_handle right;
        }expr;
        operand value;
    };

    expression() = default;

    // Initialize expression with a final value
    expression(operand_type type, u64 val) {
        this->is_value = true;
        this->value.type = type;
        if (type == OPERAND_REGISTER) {
            this->value.reg = val;
        } else {
            this->value.imm = val;
        }
    }

    expression(pool_t* iml_pool, const expression& left, math_op op, const expression& right) {
        this->expr = {
            .op = op,
            .left  = pool_push(iml_pool, &left, sizeof(left), 0),
            .right = pool_push(iml_pool, &right, sizeof(right), 0),
        };
    }
};

// Specialized format for instructions that operate on simple integer data
// (single destination, non-SIMD)
typedef struct {
    // Destinations in this type of instruction are always a register
    u8 dest_register;
    bool set_flags; // Whether to set CPU state flags with operation result
    expression expr;
}instr_math;

// Different variants of instruction
typedef enum {
    VARIANT_MATH,
    VARIANT_LOAD_STORE,
}variant;

typedef struct {
    variant var;
    union {
        instr_math math;
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
}instruction;

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
}program;

program imlgen(u8* arm_code, u32 size);

} // namespace iml
