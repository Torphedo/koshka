#include <stdlib.h>

#include "common/vfile.h"
#include "common/types.h"
#include "common/logging.h"
#include "common/file.h"

#include "os/nro.h"

#include "emit.h"

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

    // We assume the file just has ARM assembly
    vfile native_code = translate_file(path);
    return 0;
}

