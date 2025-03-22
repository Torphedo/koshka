// Page numbers in this file reference the same ISA manual as the rest of the code
#include "decode.hxx"

#include <assert.h>
#include <stdbool.h>
#include <malloc.h>

#include <common/int.h>
#include <common/logging.h>
#include <common/vfile.h>
#include <common/file.h>
#include <common/queue.h>

#include <pool.h>
#include <bitmanip.h>
#include <arm_asl.h>
#include "arm_encoding.h"
#include "iml.hxx"

namespace iml {

void decode_branch(program* prog, u32 instr) {
    LOG_MSG(debug, "Branch instruction 0x%08X\n", instr);
    // See C4.3, pg. C4-197 for the table defining all these values & cases.
    const u8 op0 = GET_BIT_REGION(instr, 29, 31);
    const u8 op1 = GET_BIT_REGION(instr, 22, 25);

    switch (op0) {
    case 0b010:
        if ((op1 & 0b1000) == 0) {
            LOG_MSG(debug, "Conditional branch (imm)\n");
        }
        break;
    case 0b110:
        if ((op1 & 0b1100) == 0) {
            LOG_MSG(debug, "Application exception\n");
        } else if (op1 == 0b0100) {
            LOG_MSG(debug, "System branch\n");
        } else if (op1 & 0b1000) {
            LOG_MSG(debug, "Unconditional branch (reg)\n");
        }
        break;
    default:
        break;
    }
    switch (op0 & 0b011) {
    case 0b000: {
        LOG_MSG(debug, "Unconditional branch (imm)\n");
        bool call = op0 & 0b100; // Top bit indicates if it's a subroutine call
        // Least significant 26 bits * 4. See C6.6.20, pg. C6-463
        // 32-bit max is 64x the 26-bit max, so multiplying by 4 is fine.
        const s32 dest = (instr & ~(0b111111 << 26)) * 4;
        LOG_MSG(info, "b");
        if (call) {
            printf("l");
        }
        printf(" #%d\n", dest);
        break;
    }

    case 0b001:
        if ((op1 & 0b1000) == 0) {
            LOG_MSG(debug, "Compare & branch (imm)\n");
        } else {
            LOG_MSG(debug, "Test & branch (imm)\n");
        }
    default:
        break;
    }
}

void decode_movw(program* prog, u32 instr) {
    const bool is_64bit = GET_SINGLE_BIT(instr, 31);
    const u8 opc = GET_BIT_REGION(instr, 29, 30);
    const u8 register_num = GET_BIT_REGION(instr, 0, 4);
    if (is_64bit) {
        switch (opc) {
        case 0b00:
            LOG_MSG(debug, "MOVN\n");
            break;
        case 0b10:
            LOG_MSG(debug, "MOVZ\n");
            break;
        case 0b11:
            LOG_MSG(debug, "MOVK\n");
            break;
        default:
            LOG_MSG(debug, "INVALID (opc 0x%x)\n", opc);
            break;
        }
    }
}

void decode_addsub_imm(program* prog, u32 instr) {
    // These names match the spec on pg. C4-193, except "sf" and "S" which are renamed
    const bool is_64bit = GET_SINGLE_BIT(instr, 31);
    const u8 op = GET_SINGLE_BIT(instr, 30);
    const bool set_flags = GET_SINGLE_BIT(instr, 29);
    const u8 shift = GET_BIT_REGION(instr, 22, 23);
    u32 imm = GET_BIT_REGION(instr, 10, 21);
    const u8 Rn = GET_BIT_REGION(instr, 5, 9);
    const u8 Rd = GET_BIT_REGION(instr, 0, 4);

    const expression expr_reg = expression(OPERAND_REGISTER, Rn);
    expression expr_imm = expression(OPERAND_IMMEDIATE, imm);
    // Immediate can be optionally left-shifted by 12 bits
    if (shift == 0b01) {
        // Insert a left-shift expression in place of the constant immediate
        const expression expr_shift = expression( OPERAND_IMMEDIATE, 12);
        expr_imm = expression(&prog->iml_pool, expr_imm, MATH_OP_LSL, expr_shift);
    }

    const math_op operation = op ? MATH_OP_ADD : MATH_OP_SUB;
    const instruction iml = {
        .var = VARIANT_MATH,
        .math = {
            .dest_register = Rd,
            .set_flags = set_flags,
            .expr = expression(&prog->iml_pool, expr_reg, operation, expr_imm),
        },
        .is_64bit = is_64bit,
    };

    // Add the IML to the pool
    pool_push(&prog->iml_pool, &iml, sizeof(iml), 0);
}

void decode_logical_imm(program* prog, u32 instr) {
    // Parse instruction fields
    const bool is_64bit = GET_SINGLE_BIT(instr, 31);
    const u8 op = GET_BIT_REGION(instr, 29, 30);
    const u8 N = GET_SINGLE_BIT(instr, 22);
    const u32 immr = GET_BIT_REGION(instr, 16, 21);
    const u32 imms = GET_BIT_REGION(instr, 10, 15);
    const u8 Rn = GET_BIT_REGION(instr, 5, 9);
    const u8 Rd = GET_BIT_REGION(instr, 0, 4);

    // TODO: Spec says 32-bit applies when sf == 0 && N == 0. (section C6.2.11)
    // Does this invalid case happen in reality?
    if (!is_64bit) {
        assert(N == 0);
    }

    const u32 imm_val = DecodeBitMasks(N, imms, immr, true);

    // We just use the operation value as a lookup table index
    const math_op operation = bitwise_op_table[op];
    const bool set_flags = (op == 0b11); // Special case

    const expression expr_reg = expression(OPERAND_REGISTER, Rn);
    const expression expr_imm = expression(OPERAND_IMMEDIATE, imm_val);

    const instruction iml = {
        .var = VARIANT_MATH,
        .math = {
            .dest_register = Rd,
            .set_flags = set_flags,
            .expr = expression(&prog->iml_pool, expr_reg, operation, expr_imm),
        },
        .is_64bit = is_64bit,
        .touched_negative_flag = set_flags,
        .touched_zero_flag = set_flags,
        // TODO: I'm not sure of the psuedocode syntax in the spec, so not sure
        // about this flag. This should be double-checked.
        // These are just set to 0
        .touched_carry_flag = set_flags,
        .touched_overflow_flag = set_flags,
    };

    pool_push(&prog->iml_pool, &iml, sizeof(iml), 0);
}

void decode_data_imm(program* prog, u32 instr) {
    LOG_MSG(debug, "Immediate instruction 0x%08X\n", instr);
    switch (data_imm_get_group(instr)) {
    case L2_DATA_IMM_MOV_WIDE:
        decode_movw(prog, instr);
        break;
    case L2_DATA_IMM_ADDSUB_IMM:
        decode_addsub_imm(prog, instr);
        break;
    case L2_DATA_IMM_LOGICAL_IMM:
        decode_logical_imm(prog, instr);
        break;
    case L2_DATA_IMM_PC_REL_ADDR:
    case L2_DATA_IMM_BITFIELD:
    case L2_DATA_IMM_EXTRACT:
        LOG_MSG(warning, "Unimplemented\n");
        break;
    case L2_DATA_IMM_UNALLOCATED:
    default:
        LOG_MSG(warning, "Unallocated/invalid instruction.\n");
        break;
    }
}

void decode_data_reg_logical_shift(program* prog, u32 instr) {
    // Parse instruction fields
    const u8 Rd        = GET_BIT_REGION(instr,  0,  4); // Destination
    const u8 Rn        = GET_BIT_REGION(instr,  5,  9); // A source register
    const u8 shift_len = GET_BIT_REGION(instr, 10, 15); // Optional shift amount
    const u8 Rm        = GET_BIT_REGION(instr, 16, 20); // A source register
    const bool negate  = GET_SINGLE_BIT(instr, 21);     // Optionally negate 2nd source reg

    // These values indicate shift and bitwise op types in a lookup table.
    const math_op shift_type = shift_type_table[GET_BIT_REGION(instr, 22, 23)];
    const math_op bitwise_op = bitwise_op_table[GET_BIT_REGION(instr, 29, 30)];
    const bool is_64bit = GET_SINGLE_BIT(instr, 31);
    const bool set_flags = GET_BIT_REGION(instr, 29, 30) == 0b11;

    if (!is_64bit) {
        assert(!GET_SINGLE_BIT(shift_len, 5) && "Can't shift a 32-bit register > 32 bits!\n");
    }

    const expression expr_Rn = expression(OPERAND_REGISTER, Rn);
    expression expr_Rm = expression(OPERAND_REGISTER, Rm);
    if (shift_len > 0) {
        // Wrap the register value in a shift expression
        expr_Rm = expression(&prog->iml_pool, expr_Rm, shift_type, expression(OPERAND_IMMEDIATE, shift_len));
    }
    if (negate) {
        // Wrap the current expression in a negation
        // Negation is unary, so the right side is empty
        expr_Rm = expression(&prog->iml_pool, expr_Rm, MATH_OP_NEG, expression());
    }

    const instruction iml = {
        .var = VARIANT_MATH,
        .math = {
            .dest_register = Rd,
            .set_flags = set_flags,
            .expr = expression(&prog->iml_pool, expr_Rn, bitwise_op, expr_Rm),
        },
        .is_64bit = is_64bit,
        .touched_negative_flag = set_flags,
        .touched_zero_flag = set_flags,
        .touched_carry_flag = set_flags,
        .touched_overflow_flag = set_flags,
    };

    pool_push(&prog->iml_pool, &iml, sizeof(iml), 0);
}

void decode_data_reg_addsub_shift(program* prog, u32 instr) {
    // TODO: This shares a lot of decoding with bitwise ops, can we merge them?
    const u8 Rd        = GET_BIT_REGION(instr,  0,  4); // Destination
    const u8 Rn        = GET_BIT_REGION(instr,  5,  9); // A source register
    const u8 shift_len = GET_BIT_REGION(instr, 10, 15); // Optional shift amount
    const u8 Rm        = GET_BIT_REGION(instr, 16, 20); // A source register

    const math_op shift_type = shift_type_table[GET_BIT_REGION(instr, 22, 23)];
    const bool set_flags = GET_SINGLE_BIT(instr, 29);
    const bool op        = GET_SINGLE_BIT(instr, 30); // true = add, false = subtract
    const bool is_64bit  = GET_SINGLE_BIT(instr, 31);
    assert(shift_type != 0b11 && "Invalid shift type!");
    if (!is_64bit) {
        assert(!GET_SINGLE_BIT(shift_len, 5) && "Can't shift a 32-bit register > 32 bits!\n");
    }

    // TODO: This is nearly identical IML building to bitwise ops, can we merge them?
    const expression expr_Rn = expression(OPERAND_REGISTER, Rn);
    expression expr_Rm = expression(OPERAND_REGISTER, Rm);
    if (shift_len > 0) {
        // Wrap the register value in a shift expression
        expr_Rm = expression(&prog->iml_pool, expr_Rm, shift_type, expression(OPERAND_IMMEDIATE, shift_len));
    }

    const instruction iml = {
        .var = VARIANT_MATH,
        .math = {
            .dest_register = Rd,
            .set_flags = set_flags,
            .expr = expression(&prog->iml_pool, expr_Rn, op ? MATH_OP_ADD : MATH_OP_SUB, expr_Rm),
        },
        .is_64bit = is_64bit,
        .touched_negative_flag = set_flags,
        .touched_zero_flag = set_flags,
        .touched_carry_flag = set_flags,
        .touched_overflow_flag = set_flags,
    };

    pool_push(&prog->iml_pool, &iml, sizeof(iml), 0);
}

void decode_data_reg(program* prog, u32 instr) {
    LOG_MSG(debug, "Register data instruction 0x%08X\n", instr);
    switch (data_reg_get_group(instr)) {
    case L2_DATA_REG_3_SOURCES:
        LOG_MSG(debug, "Data operation on 3 sources\n");
        break;
    case L2_DATA_REG_2_SOURCES:
        LOG_MSG(debug, "Data operation on 2 sources\n");
        break;
    case L2_DATA_REG_1_SOURCE:
        LOG_MSG(debug, "Data operation on 1 source\n");
        break;
    case L2_DATA_REG_LOGICAL_SHIFT:
        LOG_MSG(debug, "Bitwise instruction w/ shifted register\n");
        decode_data_reg_logical_shift(prog, instr);
        break;
    case L2_DATA_REG_ADDSUB_SHIFT:
        LOG_MSG(debug, "Add/subtract w/ shifted register\n");
        decode_data_reg_addsub_shift(prog, instr);
        break;
    case L2_DATA_REG_ADDSUB_EXTEND:
        LOG_MSG(debug, "Add/subtract w/ sign/zero-extended register\n");
        break;
    case L2_DATA_REG_ADDSUB_CARRY:
        LOG_MSG(debug, "Add/subtract w/ carry\n");
        break;
    case L2_DATA_REG_COND_COMP_REG:
        LOG_MSG(debug, "Conditional compare w/ register\n");
        break;
    case L2_DATA_REG_COND_COMP_IMM:
        LOG_MSG(debug, "Conditional compare w/ immediate\n");
        break;
    case L2_DATA_REG_COND_SEL:
        LOG_MSG(debug, "Conditional select\n");
        break;
    default:
        LOG_MSG(debug, "Invalid instruction group.\n");
        break;
    }
}

void decode_load_store(program* prog, u32 instr) {
    LOG_MSG(warning, "Unimplemented instruction 0x%08X\n", instr);
}

bdest decode(program* prog, u32 instr) {
    assert(prog != NULL);
    assert(instr != 0);
    switch (instr_get_group(instr)) {
    case L1_BRANCH:
        // TODO: Return branch destinations
        decode_branch(prog, instr);
        break;
    case L1_DATA_IMM:
        decode_data_imm(prog, instr);
        break;
    case L1_DATA_REG:
        decode_data_reg(prog, instr);
        break;
    case L1_LD_STR:
        decode_load_store(prog, instr);
        break;
    case L1_DATA_SIMD:
        LOG_MSG(warning, "Unimplemented SIMD instruction 0x%08X\n", instr);
        break;
    case L1_UNALLOCATED:
    default:
        LOG_MSG(error, "Invalid instruction 0x%08X\n", instr);
        break;
    };

    // No branch destination
    return (bdest){0};
}

} // namespace iml
