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
    if (!path_is_file(path)) {
        LOG_MSG(error, "Your input\"%s\" isn't a file\n", path);
        return 1;
    }

    u32 in_size = file_size(path);
    u32 out_size = in_size * 4;
    
    vfile native_out = vfile_open(calloc(1, out_size), out_size);
    u8* arm_code = file_load(path);
    if (native_out.ptr == NULL || arm_code == NULL) {
        LOG_MSG(error, "Failed to alloc for x86 or ARM code\n", out_size);
        free(native_out.ptr);
        free(arm_code);
        return 1;
    }

    vfile in_stream = vfile_open(arm_code, in_size);
    buf_translate(&in_stream, &native_out);

    return 0;
}

