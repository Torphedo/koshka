#pragma once
#include <common/vfile.h>
#include "iml.hxx"

namespace iml {

// TODO: Once we know for sure how branch destinations are handled, remove this
// layer of indirection in the result
typedef struct {
    // The IML instruction that was decoded
    instruction instr;
}decode_result;

decode_result decode(program* prog, u32 instr);

}
