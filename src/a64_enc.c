#include "a64_enc.h"

// See pg. C4-192.
#define A64_OP0(instr) ((instr & (0b1111 << 25)) >> 25)

#define A64_GROUP_DATA_IMM(instr) ((A64_OP0(instr) & 0b1110) == 0b1000)
#define A64_GROUP_BRANCH(instr) ((A64_OP0(instr) & 0b1110) == 0b1010)
#define A64_GROUP_LD_STR(instr) ((A64_OP0(instr) & 0b0101) == 0b0100)
#define A64_GROUP_DATA_REG(instr) ((A64_OP0(instr) & 0b0111) == 0b0101)
// ISA manual specifies 0b1111 && 0b0111 case, even though they're the same?
#define A64_GROUP_DATA_SIMD(instr) ((A64_OP0(instr) & 0b0111) == 0b0111)

enc_cat instr_get_group(u32 instr) {
    if (A64_GROUP_DATA_IMM(instr)) { return DATA_IMM; } 
    else if (A64_GROUP_BRANCH(instr)) { return BRANCH; }
    else if (A64_GROUP_LD_STR(instr)) { return LD_STR; }
    else if (A64_GROUP_DATA_REG(instr)) { return DATA_REG; }
    else if (A64_GROUP_DATA_SIMD(instr)) { return DATA_SIMD; }

    return UNALLOCATED;
}

// See C4.2 on pg. C4-193
#define DATA_IMM_OP0(instr) ((instr & (0b111 << 23)) >> 23)
#define DATA_IMM_PC_REL(instr) ((DATA_IMM_OP0(instr) & 0b110) == 0b000)
#define DATA_IMM_ADDSUB(instr) ((DATA_IMM_OP0(instr) & 0b110) == 0b010)
#define DATA_IMM_LOGICAL(instr) (DATA_IMM_OP0(instr) == 0b100)
#define DATA_IMM_MOVW(instr) (DATA_IMM_OP0(instr) == 0b101)
#define DATA_IMM_BITFIELD(instr) (DATA_IMM_OP0(instr) == 0b110)
#define DATA_IMM_EXTRACT(instr) (DATA_IMM_OP0(instr) == 0b111)

data_imm_cat data_imm_get_group(u32 instr) {
    if (DATA_IMM_PC_REL(instr)) { return pc_rel_adr; } 
    else if (DATA_IMM_ADDSUB(instr)) { return addsub_imm; }
    else if (DATA_IMM_LOGICAL(instr)) { return logical_imm; }
    else if (DATA_IMM_MOVW(instr)) { return mov_wide; }
    else if (DATA_IMM_BITFIELD(instr)) { return bitfield; }
    else if (DATA_IMM_EXTRACT(instr)) { return extract; }

    return 0;
}

