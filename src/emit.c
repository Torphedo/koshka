#include <stdbool.h>

#include "emit.h"
#include "logging.h"

#include "a64_enc.h"
#include "regalloc.h"
#include "struct/vfile.h"

void emit_branch(vfile* out, u32 instr) {
    LOG_MSG(debug, "Branch instruction 0x%08X\n", instr);
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

void emit(vfile* out, u32 instr) {
    switch (instr_get_group(instr)) {
    case BRANCH:
        emit_branch(out, instr);
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
    while (!vfile_eof(src)) {
        u32 instr = VFILE_READ(u32, src);
        if (vfile_eof(dest)) {
            LOG_MSG(error, "Hit virtual EOF during translation!\n\n");
            no_errors = false;
            break;
        }
        emit(dest, instr);
    }
    if (no_errors) {
        LOG_MSG(info, "Finished translating buffer with no vfile issues.\n");
    }
}

