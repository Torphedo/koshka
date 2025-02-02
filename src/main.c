#include <stdlib.h>

#include <common/vfile.h>
#include <common/path.h>
#include <common/int.h>
#include <common/logging.h>
#include <common/file.h>

#include "os/nro.h"

#include "imlgen/decode.h"
#include "elf.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        LOG_MSG(info, "No filename provided\n");
        LOG_MSG(info, "Usage: koshka [ARMv8 file]\n");
        return 1;
    }
    char* path = argv[1];
    if (path_has_extension(path, ".nro")) {
        module program = nro_load("illuminatiNX.nro");
        return 0;
    }

    if (file_is_elf(path)) {
        load_elf(path);
        return EXIT_SUCCESS;
    }

    const s64 size = file_size(path);
    u8* arm_buf = file_load(path);
    if (arm_buf == NULL) {
        LOG_MSG(error, "Failed to load ARM assembly file \"%s\"\n", path);
        return EXIT_FAILURE;
    }

    // We assume the file just has ARM assembly
    vfile arm_code = vfile_open(arm_buf, size);
    while (vfile_opcheck(&arm_code, sizeof(u32))) {
        const u32 arm_instr = VFILE_READ(u32, &arm_code);
        const iml_instr iml = decode(arm_instr);
    }

    return EXIT_SUCCESS;
}
