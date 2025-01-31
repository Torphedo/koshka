// Page numbers in this file reference the same ISA manual as in a64_enc.h.
#include "decode.h"

#include <stdbool.h>
#include <malloc.h>

#include <common/int.h>
#include <common/logging.h>
#include <common/vfile.h>
#include <common/file.h>
#include <common/queue.h>

#include <regalloc.h>
#include "arm_encoding.h"

iml_instr decode_branch(u32 instr) {
    iml_instr out = {.operation = IML_OP_BRANCH};
    LOG_MSG(debug, "Branch instruction 0x%08X\n", instr);
    // See C4.3, pg. C4-197 for the table defining all these values & cases.
    u8 op0 = instr >> 29;
    u8 op1 = (instr >> 22) & 0xF;

    switch (op0) {
    case 0b010:
        if ((op1 & 0b1000) == 0) {
            LOG_MSG(debug, "Conditional branch (imm)\n");
            out.branch_info.type = IML_BRANCH_CONDITIONAL;
        }
        break;
    case 0b110:
        out.branch_info.type = IML_BRANCH_UNCONDITIONAL;
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
    case 0b000:
        LOG_MSG(debug, "Unconditional branch (imm)\n");
        bool call = op0 & 0b100; // Top bit indicates if it's a subroutine call
        // Least significant 26 bits * 4. See C6.6.20, pg. C6-463
        // 32-bit max is 64x the 26-bit max, so multiplying by 4 is fine.
        s32 dest = (instr & ~(0b111111 << 26)) * 4;
        LOG_MSG(info, "b");
        if (call) {
            printf("l");
        }
        printf(" #%d\n", dest);

        out.branch_info.type = IML_BRANCH_UNCONDITIONAL;
    case 0b001:
        if ((op1 & 0b1000) == 0) {
            LOG_MSG(debug, "Compare & branch (imm)\n");
        } else {
            LOG_MSG(debug, "Test & branch (imm)\n");
        }
        out.branch_info.type = IML_BRANCH_CONDITIONAL;
    default:
        break;
    }

    // This can only be reached if it reaches none of the valid cases.
    return out;
}

iml_instr decode_movw(u32 instr) {
    iml_instr out = {0};
    const bool is_64bit = ((instr & (1 << 31)) != 0);
    const u8 opc = (instr & (0b11 << 27)) >> 27;
    if (is_64bit) {
        switch (opc) {
        case 0b00:
            LOG_MSG(debug, "MOVN\n");
            break;
        case 0b10:
            LOG_MSG(debug, "MOVZ\n");
            const u8 id = reg_alloc(instr & 0b1111);
            const u64 imm = (instr & (0xFFFF << 5)) >> 5;
            out.op1.type = IML_OPERAND_REGISTER;
            out.op1.reg = id;

            out.op2.type = IML_OPERAND_IMMEDIATE;
            out.op2.reg = imm;
            break;
        case 0b11:
            LOG_MSG(debug, "MOVK\n");
            break;
        default:
            LOG_MSG(debug, "INVALID\n");
            break;
        }
    }

    return out;
}

iml_instr decode_data_imm(u32 instr) {
    LOG_MSG(debug, "Immediate data instruction 0x%08X\n", instr);
    switch (data_imm_get_group(instr)) {
    case mov_wide:
        decode_movw(instr);
        break;
    default:
        LOG_MSG(warning, "Unimplemented\n");
        break;
    }
    return (iml_instr){0};
}

iml_instr decode_data_reg(u32 instr) {
    LOG_MSG(debug, "Register data instruction 0x%08X\n", instr);
    return (iml_instr){0};
}

iml_instr decode_load_store(u32 instr) {
    LOG_MSG(warning, "Unimplemented load/store 0x%08X\n", instr);
    return (iml_instr){0};
}

iml_instr decode(u32 instr) {
    switch (instr_get_group(instr)) {
    case BRANCH:
        return decode_branch(instr);
        break;
    case DATA_IMM:
        return decode_data_imm(instr);
        break;
    case DATA_REG:
        return decode_data_reg(instr);
        break;
    case LD_STR:
        return decode_load_store(instr);
        break;
    case DATA_SIMD:
        LOG_MSG(warning, "Unimplemented SIMD instruction 0x%08X\n", instr);
        break;
    case UNALLOCATED:
        LOG_MSG(error, "Invalid instruction 0x%08X\n", instr);
        break;
    default:
        LOG_MSG(warning, "Unimplemented instruction group 0x%08X\n", instr);
        break;
    };

    // Something went wrong...
    return (iml_instr){0};
}
