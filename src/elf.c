#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <elf_structure.h>
#include <common/int.h>
#include <common/vfile.h>
#include <common/logging.h>

#include "imlgen/decode.h"
#include "bitmanip.h"

bool file_is_elf(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }

    Elf64_Ehdr header = {0};
    fread(&header, sizeof(header), 1, f);

    if (strncmp((char*)&header, ELFMAG, SELFMAG) == 0) {
        return true;
    }

    return false;
}

u8* load_elf(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }

    Elf64_Ehdr header = {0};
    fread(&header, sizeof(header), 1, f);
    if (strncmp((char*)&header, ELFMAG, SELFMAG) != 0) {
        LOG_MSG(error, "\"%s\" doesn't look like an ELF file!\n", path);
        fclose(f);
        return NULL;
    }
    if (header.e_machine != EM_AARCH64) {
        LOG_MSG(error, "\"%s\" isn't for AArch64!\n", path);
        fclose(f);
        return NULL;
    }

    for (u32 i = 0; i < header.e_shnum; i++) {
        // Jump to next program header
        Elf64_Shdr prog_header = {0};
        fseek(f, header.e_shoff + sizeof(prog_header) * i, SEEK_SET);

        fread(&prog_header, sizeof(prog_header), 1, f);
        const s64 size = prog_header.sh_size;
        if (prog_header.sh_type != SHT_PROGBITS) {
            continue;
        }

        if (!HAS_BIT_FLAG(prog_header.sh_flags, SHF_EXECINSTR)) {
            continue;
        }

        if (size < 4) {
            LOG_MSG(warning, "Skipping section %u, (too small to have code)!\n", i);
            continue;
        }


        // Jump to section
        fseek(f, prog_header.sh_offset, SEEK_SET);
        LOG_MSG(debug, "Decoding 0x%x-byte section @ 0x%x\n", size, prog_header.sh_offset);

        // Allocate & read code
        u8* buf = calloc(1, size);
        if (buf == NULL) {
            LOG_MSG(error, "Failed to allocate 0x%x bytes for ARM code!\n", size);
            fclose(f);
            return NULL;
        }

        fread(buf, size, 1, f);
        // We assume the file just has ARM assembly
        const iml_program prog = imlgen(buf, size);
        free(buf);
    }

    return NULL;
}
