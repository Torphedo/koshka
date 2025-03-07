#include "iml.h"
#include "decode.h"
#include <pool.h>

#include <stdbool.h>

void imlgen_recurse(iml_program* prog, pool_handle pos) {
    const u32* arm_instr = pool_getdata(prog->arm_pool, pos);
    // Decoding function returns branch destinations if they exist
    const s64 branch_dest = decode(prog, *arm_instr);

    // Decode the rest of the current function before we resolve branches, to
    // keep more of the IML in one place.
    imlgen_recurse(prog, pos + 4);
    if (branch_dest != -1) {
        imlgen_recurse(prog, branch_dest);
    }
}

iml_program imlgen(u8* arm_code, u32 size) {
    iml_program out = {
        .instruction_pool = pool_open(50 * sizeof(iml_instr)),
        .arm_pool = pool_open(size),
    };

   imlgen_recurse(&out, 0);

    return out;
}

