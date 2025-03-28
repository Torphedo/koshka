#pragma once
// A less architecture-dependent representation of ARM code
//
// Takes heavy inspiration from the Cemu PPC Recompiler:
// https://github.com/cemu-project/Cemu/tree/main/src/Cafe/HW/Espresso/Recompiler
// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompiler.h
// https://github.com/cemu-project/Cemu/blob/main/src/Cafe/HW/Espresso/Recompiler/PPCRecompilerIml.h

#include "bitmanip.h"
#include <common/int.h>
#include <common/list.h>
#include <pool.h>
#include <boolset.hxx>

namespace iml {

// All potential math operations (non-SIMD)
enum math_op : u8 {
    // TODO: Consider adding separate ops for add w/ carry or signed addition

    // Basic 4-function ops
    MATH_OP_ADD,
    MATH_OP_SUB,
    MATH_OP_MUL,
    MATH_OP_DIV,

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

// Equivalent to the ASL function DecodeShift()
static const math_op shift_type_table[] = {
    MATH_OP_LSL,
    MATH_OP_LSR,
    MATH_OP_ASR,
    MATH_OP_ROR,
};

// Maps [opc] field to an operation, for shifted bitwise ops in section C4.5.10
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

// A single operand to an expression
struct operand {
    // Operand can be either a register or immediate value
    operand_type type: 2;
    union {
        u8 reg; // Register ID

        // TODO: Find out the largest immediate size and shrink this
        u64 imm;
    };
};

// Many instructions will operate on a register value before using it in
// another operation, but don't mutate the register itself.
// For example BIC (bitwise clear) can encode things like this:
//                    Rd = Rn & (~(Rm >> 7))
//  ... but only Rd is mutated.
// To handle this, we use a recursive expression tree.

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
            bool signed_op; // "signed" on its own is a reserved keyword
            bool carry;

            // The left and right sub-expressions
            expression_handle left;
            expression_handle right;
        }expr;

        // This is a container type that distinguishes registers vs. immediates
        operand value;
    };

    expression() = default;

    // Initialize expression with a final value
    /// @brief Create a constant value expression
    /// @param type The type of value (register or immediate)
    /// @param val The constant value to store
    /// @param force_register References to the special zero register will get
    /// replaced with the immediate value 0, unless this argument is true.
    expression(operand_type type, u64 val, bool force_register = false);

    /// @brief Add a left and right expression to a pool, and construct an expression between them
    ///
    /// @param iml_pool The IML pool to store your expressions in
    /// @param left The left side of the new expression
    /// @param op   The operation to perform (e.g. add/subtract). For unary
    /// operations (like bitwise negate), leave the right expression blank.
    /// @param right The right side of the new expression
    /// @param carry TODO: Document this more precisely
    expression(pool_t* iml_pool, const expression& left, math_op op, const expression& right, bool carry = false);
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

enum iml_register : u8 {
    // The rest are regular integers representing r1-r31

    // Special value used in ARM, a register that always reads as 0
    REGISTER_ZERO = MAX_VAL_FOR_SIZE(5),
    REGISTER_PC = REGISTER_ZERO + 1,
};

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

// Tree of IML instructions
typedef struct {
    // We use a pool for the entire tree so it's easily destroyed, and branches
    // can reference their destination in a simple arch-independent way
    pool_t iml_pool;
    pool_t arm_pool; // Pool of ARM instructions
    pool_handle entry_point; // The first instruction

    // All known branch destinations that have already been decoded
    boolset decoded_instrs;
}program;

program imlgen(u8* arm_code, u32 size);

} // namespace iml
