// Page numbers in this file reference the same ISA manual as in a64_enc.h.

#include <stdbool.h>

#include "common/types.h"
#include "common/logging.h"
#include "common/vfile.h"
#include "common/queue.h"

#include "a64_enc.h"
#include "regalloc.h"

// Temporary tracker variables for debugging.
u32 branch_cond = 0;
u32 exception = 0;
u32 system = 0;
u32 uncond_reg = 0;
u32 uncond_imm = 0;
u32 compare_branch = 0;
u32 test_branch = 0;

void emit_branch(vfile* in, vfile* out, queue* branch_q, u32 instr) {
    LOG_MSG(debug, "Branch instruction 0x%08X\n", instr);
    // See C4.3, pg. C4-197 for the table defining all these values & cases.
    u8 op0 = instr >> 29;
    u8 op1 = (instr >> 22) & 0xF;
    
    switch (op0) {
    case 0b010:
        if ((op1 & 0b1000) == 0) {
            LOG_MSG(debug, "Conditional branch (imm)\n");
            branch_cond++;
            return;
        }
        break;
    case 0b110:
        if ((op1 & 0b1100) == 0) {
            LOG_MSG(debug, "Application exception\n");
            exception++;
            return;
        } else if (op1 == 0b0100) {
            LOG_MSG(debug, "System branch\n");
            system++;
            return;
        } else if (op1 & 0b1000) {
            LOG_MSG(debug, "Unconditional branch (reg)\n");
            uncond_reg++;
            return;
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
        uncond_imm++;

        // TODO: For jumps (when call variable is false), we can just jump
        // ahead to the destination.
        return;
    case 0b001:
        if ((op1 & 0b1000) == 0) {
            LOG_MSG(debug, "Compare & branch (imm)\n");
            compare_branch++;
        } else {
            LOG_MSG(debug, "Test & branch (imm)\n");
            test_branch++;
        }
        return;
    default:
        break;
    }

    // This can only be reached if it reaches none of the valid cases.
    LOG_MSG(error, "Invalid instruction 0x%08X\n", instr);
    return;
}

void emit_movw(vfile* out, u32 instr) {
    // TODO: I think starting w/ an underscore is reserved? Find a better way.
    bool _64bit = ((instr & (1 << 31)) != 0);
    u8 opc = (instr & (0b11 << 27)) >> 27;
    if (_64bit) {
        switch (opc) {
        case 0b00:
            LOG_MSG(debug, "MOVN\n");
            break;
        case 0b10:
            LOG_MSG(debug, "MOVZ\n");
            u8 id = reg_alloc(instr & 0b1111);
            u64 imm = (instr & (0xFFFF << 5)) >> 5;
            VFILE_WRITE(u8, out, 0x48 + (id / 8));
            VFILE_WRITE(u8, out, 0xB8 + (id % 8));
            VFILE_WRITE(u64, out, imm);
            break;
        case 0b11:
            LOG_MSG(debug, "MOVK\n");
            break;
        default:
            LOG_MSG(debug, "INVALID\n");
            break;
        }
    }
}

void emit_data_imm(vfile* out, u32 instr) {
    LOG_MSG(debug, "Immediate data instruction 0x%08X\n", instr);
    switch (data_imm_get_group(instr)) {
    case mov_wide:
        emit_movw(out, instr);
        break;
    default:
        LOG_MSG(warning, "Unimplemented\n");
        break;
    }
    return;
}

void emit_data_reg(vfile* out, u32 instr) {
    LOG_MSG(debug, "Register data instruction 0x%08X\n", instr);
    return;
}

void emit_load_store(vfile* out, u32 instr) {
    LOG_MSG(warning, "Unimplemented load/store 0x%08X\n", instr);
}

void emit(vfile* in, vfile* out, queue* branch_q, u32 instr) {
    switch (instr_get_group(instr)) {
    case BRANCH:
        emit_branch(in, out, branch_q, instr);
        break;
    case DATA_IMM:
        emit_data_imm(out, instr);
        break;
    case DATA_REG:
        emit_data_reg(out, instr);
        break;
    case LD_STR:
        emit_load_store(out, instr);
        break;
    case DATA_SIMD:
        LOG_MSG(warning, "Unimplemented SIMD instruction 0x%08X\n", instr);
        break;
    case UNALLOCATED:
        LOG_MSG(error, "Invalid instruction 0x%08X\n", instr);
        break;
    default:
        LOG_MSG(warning, "Unimplemented instruction group 0x%08X\n", instr);
        return;
    };
}

// TODO: Follow branches while translating, & add the destination of
// conditional branches to a queue. I guess we have to check each time if
// the current address is in the queue and remove it? Seems inefficient...


// We can't know the final address of an untranslated function when
// translating a function call. Cemu's solution of an indirect jump via
// lookup table may be a good idea.
// The dereference from indirect jmp might hurt loops, worth benchmarking.
// If jmp & call are the same size, maybe we could go back and replace
// these with direct calls after translation?

void buf_translate(vfile* src, vfile* dest) {
    bool no_errors = true;
    // TODO: Could we make this a queue of u32 offsets?
    queue branch_q = queue_create(0x40);
    while (!vfile_eof(src)) {
        u32 instr = VFILE_READ(u32, src);
        if (vfile_eof(dest)) {
            LOG_MSG(error, "Hit virtual EOF during translation!\n\n");
            no_errors = false;
            break;
        }
        emit(src, dest, &branch_q, instr);
    }

    while (!queue_empty(&branch_q)) {
        u64 instr_ptr = queue_get(&branch_q);
        src->pos = (u32)instr_ptr;

        buf_translate(src, dest);
    }

    if (no_errors) {
        LOG_MSG(info, "Finished translating buffer with no vfile issues.\n");
    }

    LOG_MSG(debug, "Conditional branch: %d\n", branch_cond);
    LOG_MSG(debug, "Application exception: %d\n", exception);
    LOG_MSG(debug, "System branch: %d\n", system);
    LOG_MSG(debug, "Unconditional branch (reg): %d\n", uncond_reg);
    LOG_MSG(debug, "Unconditional branch (imm): %d\n", uncond_imm);
    LOG_MSG(debug, "Compare & branch: %d\n", compare_branch);
    LOG_MSG(debug, "Test & branch: %d\n", test_branch);
}

