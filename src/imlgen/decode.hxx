#pragma once
#include <common/vfile.h>
#include "iml.hxx"

namespace iml {

typedef struct {
    // Constant or variable expression for the branch destination
    expression branch_dest;
    // The IML instruction that was decoded
    instruction instr;
}decode_result;

decode_result decode(program* prog, u32 instr);

}
