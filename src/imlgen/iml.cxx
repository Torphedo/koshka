#include "iml.hxx"
#include "decode.hxx"
#include <common/list.h>
#include <pool.h>
#include <bitmanip.h>

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
        // Mark this location as decoded
        prog->decoded_instrs.set_bit(pos / 4, 1);
    } // else we're at the end of the ARM buffer

    // Last instruction was a branch, decode its destination (if we haven't already)

    const bool dest_already_decoded = prog->decoded_instrs.get_bit(pos / 4);
    if (branch.exists && !dest_already_decoded) {
        const u32 dest = branch.dest;
        imlgen_recurse(prog, dest);

        // Add to list of decoded destinations
        prog->decoded_instrs.set_bit(dest / 4, 1);
    }
}

program imlgen(u8* arm_code, u32 size) {
    assert(arm_code != NULL);
    assert(size > 0);
    const u32 num_instrs = (size / 4);
    program out = {
        .iml_pool = pool_open(num_instrs * sizeof(instruction)),
        .arm_pool = {
            .data = (uintptr_t)arm_code,
            .alloc_size = size,
        },
        .decoded_instrs = boolset(num_instrs),
    };

   imlgen_recurse(&out, 0);

    return out;
}

expression::expression(operand_type type, u64 val, bool force_register) {
    this->is_value = true;
    this->value.type = type;
    switch (type) {
    case OPERAND_REGISTER:
        if (val == REGISTER_ZERO && !force_register) {
            // We can replace the zero register with an immediate 0. The caller
            // can override this (e.g. if in their context it means the stack
            // pointer instead).
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
