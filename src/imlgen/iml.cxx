#include "iml.hxx"
#include "decode.hxx"
#include <common/list.h>
#include <pool.h>
#include <bitmanip.h>

#include <assert.h>
#include <stdbool.h>

namespace iml {

pool_handle imlgen_recurse(program* prog, pool_handle pos) {
    assert(prog != NULL);
    const u32* arm_instr = (u32*)pool_getdata(prog->arm_pool, pos);
    // Decoding function returns branch destinations if they exist
    decode_result result = decode(prog, *arm_instr);

    // Push this instruction to the pool
    const pool_handle Hdecoded = pool_push(&prog->iml_pool, &result.instr, sizeof(result.instr), 0);

    // Decode the rest of the current function before we resolve branches, to
    // keep more of the IML in one place.
    if (pos + 4 < prog->arm_pool.alloc_size) {
        result.instr.next = imlgen_recurse(prog, pos + 4);
        // Mark this location as decoded
        prog->decoded_instrs.set_bit(pos / 4, 1);
    } // else we're at the end of the ARM buffer

    // Update next pointer with the result of the recursive call
    ((instruction*)pool_getdata(prog->iml_pool, Hdecoded))->next = result.instr.next;

    // Last instruction was a branch, decode its destination (if we haven't already)

    const bool dest_already_decoded = prog->decoded_instrs.get_bit(pos / 4);

    if (result.branch_dest.is_value && result.branch_dest.value.type == OPERAND_IMMEDIATE) {
        // Constant PC-relative branch
    }

    // Tell caller where we put the IML for this instruction
    return Hdecoded;
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
