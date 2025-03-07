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
#include "pool.h"

void decode_branch(iml_program* prog, u32 instr) {
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
}

void decode_movw(iml_program* prog, u32 instr) {
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
}

// Decoding for L2_DATA_IMM_ADDSUB_IMM
void decode_addsub_imm(iml_program* prog, u32 instr) {
    // These names match the spec on pg. C4-193, except "sf" and "S" which are renamed
    const bool is_64bit = GET_SINGLE_BIT(instr, 31);
    const u8 op = GET_SINGLE_BIT(instr, 30);
    const bool set_flags = GET_SINGLE_BIT(instr, 29);
    const u8 shift = GET_BIT_REGION(instr, 22, 23);
    u32 imm = GET_BIT_REGION(instr, 10, 21);
    const u8 Rn = GET_BIT_REGION(instr, 5, 9);
    const u8 Rd = GET_BIT_REGION(instr, 0, 4);

    // Immediate can be optionally shifted
    if (shift == 0b01) {
        imm <<= 12;
    }

    const iml_instr iml = {
        .is_64bit = is_64bit,
        .variant = L1_DATA_IMM,
        .data = {
            .dest_register = Rd,
            .set_flags = set_flags,
            .operation = op ? DATA_OP_ADD : DATA_OP_SUB,
            .sources[0] = {
                .type = IML_OPERAND_REGISTER,
                .reg = Rn,
                .exists = true,
            },
            .sources[1] = {
                .type = IML_OPERAND_IMMEDIATE,
                .imm = imm,
                .exists = true,
            },
        },
    };
    
    // Add the IML to the pool
    pool_push(&prog->iml_pool, &iml, sizeof(iml), sizeof(iml));
}

void decode_data_imm(iml_program* prog, u32 instr) {
    LOG_MSG(debug, "Immediate instruction 0x%08X\n", instr);
    switch (data_imm_get_group(instr)) {
    case L2_DATA_IMM_MOV_WIDE:
        decode_movw(prog, instr);
        break;
    case L2_DATA_IMM_ADDSUB_IMM:
        decode_addsub_imm(prog, instr);
        break;
    case L2_DATA_IMM_UNALLOCATED:
    case L2_DATA_IMM_PC_REL_ADDR:
    case L2_DATA_IMM_LOGICAL_IMM:
    case L2_DATA_IMM_BITFIELD:
    case L2_DATA_IMM_EXTRACT:
    default:
        LOG_MSG(warning, "Unimplemented\n");
        break;
    }
}

void decode_data_reg(iml_program* prog, u32 instr) {
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
}

void decode_load_store(iml_program* prog, u32 instr) {
    LOG_MSG(warning, "Unimplemented instruction 0x%08X\n", instr);
}

bdest decode(iml_program* prog, u32 instr) {
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
