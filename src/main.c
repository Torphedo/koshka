#include <stdlib.h>

#include <common/vfile.h>
#include <common/path.h>
#include <common/int.h>
#include <common/logging.h>
#include <common/file.h>

#include "imlgen/iml.h"
#include "os/nro.h"

#include "elf.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        LOG_MSG(info, "No filename provided\n");
        LOG_MSG(info, "Usage: koshka [ARMv8 file]\n");
        return 1;
    }
    char* path = argv[1];
    if (path_has_extension(path, ".nro")) {
        module program = nro_load(path);
        return 0;
    }

    s64 size = 0;
    u8* arm_buf = NULL;
    if (file_is_elf(path)) {
        arm_buf = load_elf(path, &size);
        if (arm_buf == NULL || size == 0) {
            LOG_MSG(error, "Couldn't load ELF file \"%s\" (or it was empty)\n");
            return EXIT_FAILURE;
        }
    } else {
        // We assume the file just has ARM assembly
        size = file_size(path);
        arm_buf = file_load(path);
        if (arm_buf == NULL || size < 1) {
            LOG_MSG(error, "Failed to load ARM assembly file \"%s\" (%lld bytes)\n", path, size);
            free(arm_buf);
            return EXIT_FAILURE;
        }
    }

    vfile arm_code = vfile_open(arm_buf, size);
    LOG_MSG(debug, "Generating IML for %d ARM instructions\n", size / 4);
    const iml_program prog = imlgen(arm_buf, size);
    // TODO: Do shatter here
    // TODO: Do register allocation here

    return EXIT_SUCCESS;
}
