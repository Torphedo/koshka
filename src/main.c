#include <stdlib.h>

#include "types.h"
#include "logging.h"
#include "emit.h"
#include "file.h"
#include "struct/vfile.h"

#include "os/nro.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        u8* text_section_out = calloc(1, 0x400 * 0x10);
        module program = nro_load("illuminatiNX.nro");
        return 1;

        LOG_MSG(info, "No filename provided\n");
        LOG_MSG(info, "Usage: koshka [ARMv8 file]\n");
        return 1;
    }
    char* path = argv[1];
    u32 size = file_size(path) * 4;
    FILE* arm_code = fopen(path, "rb");
    if (arm_code == NULL) {
        LOG_MSG(error, "Failed to open %s for reading\n", path);
        return 1;
    }
    
    vfile native_out = vfile_open(calloc(1, size), size);
    if (native_out.ptr == NULL) {
        LOG_MSG(error, "Failed to make room for %d bytes of native code\n", size);
        return 1;
    }

    u32 instr = 0;
    while (fread(&instr, 1, 4, arm_code) == 4) {
        emit(&native_out, instr);
    }
    
    return 0;
}

