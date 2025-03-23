#include "iml.hxx"
#include "decode.hxx"
#include <common/list.h>
#include <pool.h>

#include <assert.h>
#include <stdbool.h>

namespace iml {

void imlgen_recurse(program* prog, pool_handle pos) {
    assert(prog != NULL);
    const u32* arm_instr = (u32*)pool_getdata(prog->arm_pool, pos);
    // Decoding function returns branch destinations if they exist
    const bdest branch = decode(prog, *arm_instr);

    // Decode the rest of the current function before we resolve branches, to
    // keep more of the IML in one place.
    if (pos + 4 < prog->arm_pool.alloc_size) {
        imlgen_recurse(prog, pos + 4);
    } // else we're at the end of the ARM buffer

    // Last instruction was a branch, decode its destination (if we haven't already)
    // TODO: This is a linear search, replace w/ hash buckets if it's too slow
    if (branch.exists && !list_contains(prog->branch_dests, &branch.dest)) {
        const u32 dest = branch.dest;
        imlgen_recurse(prog, branch.dest);
        // Add to list of decoded destinations
        list_add(&prog->branch_dests, &branch.dest);
    }
}

program imlgen(u8* arm_code, u32 size) {
    assert(arm_code != NULL);
    assert(size > 0);
    program out = {
        .iml_pool = pool_open(50 * sizeof(instruction)),
        .arm_pool = {
            .data = (uintptr_t)arm_code,
            .alloc_size = size,
        },
        .branch_dests = list_create(50 * sizeof(s64), sizeof(s64)),
    };

   imlgen_recurse(&out, 0);

    return out;
}

expression::expression(operand_type type, u64 val, bool force_register) {
    this->is_value = true;
    this->value.type = type;
    switch (type) {
    case OPERAND_REGISTER:
        if (val == MAX_VAL_FOR_SIZE(5) && !force_register) {
            // This special value means the zero register, so replace it with
            // an immediate 0. The caller can override this (e.g. if in their
            // context it means the stack pointer instead).
            this->value.type = OPERAND_IMMEDIATE;
            this->value.imm = 0;
        } else {
            this->value.reg = val;
        }
        break;
    case OPERAND_IMMEDIATE:
        this->value.imm = val;
        break;
    }
}

expression::expression(pool_t* iml_pool, const expression& left, math_op op, const expression& right, bool carry) {
    this->expr = {
        .op = op,
        .carry = carry,
        .left  = pool_push(iml_pool, &left, sizeof(left), 0),
        .right = pool_push(iml_pool, &right, sizeof(right), 0),
    };
}

} // namespace iml
