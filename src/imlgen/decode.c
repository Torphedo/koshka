// Page numbers in this file reference the same ISA manual as the rest of the code
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
#include "bitmanip.h"

iml_instr decode_branch(u32 instr) {
    iml_instr out = {0};
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
    case 0b000:
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

    case 0b001:
        if ((op1 & 0b1000) == 0) {
            LOG_MSG(debug, "Compare & branch (imm)\n");
        } else {
            LOG_MSG(debug, "Test & branch (imm)\n");
        }
    default:
        break;
    }

    // This can only be reached if it reaches none of the valid cases.
    return out;
}

iml_instr decode_movw(u32 instr) {
    iml_instr out = {0};
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
            const u64 imm = (instr & (0xFFFF << 5)) >> 5;
            break;
        case 0b11:
            LOG_MSG(debug, "MOVK\n");
            break;
        default:
            LOG_MSG(debug, "INVALID (opc 0x%x)\n", opc);
            break;
        }
    }

    return out;
}

iml_instr decode_data_imm(u32 instr) {
    LOG_MSG(debug, "Immediate instruction 0x%08X\n", instr);
    switch (data_imm_get_group(instr)) {
    case L2_DATA_IMM_MOV_WIDE:
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
        break;
    case L2_DATA_REG_ADDSUB_SHIFT:
        LOG_MSG(debug, "Add/subtract w/ shifted register\n");
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
    return (iml_instr){0};
}

iml_instr decode_load_store(u32 instr) {
    LOG_MSG(warning, "Unimplemented instruction 0x%08X\n", instr);
    return (iml_instr){0};
}

iml_instr decode(u32 instr) {
    switch (instr_get_group(instr)) {
    case L1_BRANCH:
        return decode_branch(instr);
        break;
    case L1_DATA_IMM:
        return decode_data_imm(instr);
        break;
    case L1_DATA_REG:
        return decode_data_reg(instr);
        break;
    case L1_LD_STR:
        return decode_load_store(instr);
        break;
    case L1_DATA_SIMD:
        LOG_MSG(warning, "Unimplemented SIMD instruction 0x%08X\n", instr);
        break;
    case L1_UNALLOCATED:
    default:
        LOG_MSG(error, "Invalid instruction 0x%08X\n", instr);
        break;
    };

    // Something went wrong...
    return (iml_instr){0};
}
