#include "iml.h"
#include "common/list.h"
#include "decode.h"
#include <pool.h>

#include <assert.h>
#include <stdbool.h>

void imlgen_recurse(iml_program* prog, pool_handle pos) {
    assert(prog != NULL);
    const u32* arm_instr = pool_getdata(prog->arm_pool, pos);
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

iml_program imlgen(u8* arm_code, u32 size) {
    assert(arm_code != NULL);
    assert(size > 0);
    iml_program out = {
        .iml_pool = pool_open(50 * sizeof(iml_instr)),
        .arm_pool = {
            .data = (uintptr_t)arm_code,
            .alloc_size = size,
        },
        .branch_dests = list_create(50 * sizeof(s64), sizeof(s64)),
    };

   imlgen_recurse(&out, 0);

    return out;
}
