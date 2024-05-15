#include <stdio.h>
#include <stdlib.h>

#include <types.h>
#include <file.h>
#include <struct/vfile.h>
#include <emit.h>
#include <logging.h>
#include "nro.h"
#include "vmem.h"
#include "module.h"

typedef enum {
    TEXT = 0
}nro_seg;

module nro_load(const char* path) {
    module output = {0};
    FILE* nro = fopen(path, "rb");
    if (nro == NULL) {
        LOG_MSG(error, "Couldn't open %s\n", path);
        return output;
    }
    LOG_MSG(debug, "Opened %s\n", path);
    nro_start start = {0};
    mod0 mod = {0};
    nro_hdr header = {0};
    fread(&start, sizeof(start), 1, nro);
    fread(&header, sizeof(header), 1, nro);

    // Allocate & read .text section
    u32 textsize = header.text_seg.size;
    u32 textpos = header.text_seg.offset;
    fseek(nro, textpos, SEEK_SET);
    u8* textseg = vmem_alloc(textsize, MEM_RW);
    if (textseg == NULL) {
        LOG_MSG(error, "Couldn't alloc 0x%x bytes for .text section\n", textsize);
        return output;
    }
    fread(textseg, textsize, 1, nro);


    fseek(nro, start.mod0_offset, SEEK_SET);
    fread(&mod, sizeof(mod), 1, nro);

    vfile textsrc = vfile_open(textseg, textsize);
    // 50% larger than ARM buffer, in case code size is larger
    vfile textdest = vfile_open(vmem_alloc(textsize * 1.5, MEM_RWX), textsize * 1.5);
    buf_translate(&textsrc, &textdest);
    LOG_MSG(info, "Translated .text section\n");
    

    // MOD0 seems to be optional?? Nothing about this format makes sense.
    bool has_mod0 = (mod.magic == MAGIC('M', 'O', 'D', '0'));

    // TODO: If on ARM, just memcpy NRO into reserved region & jmp to .text?
    // Maybe filter out syscalls I guess

    // TODO: NRO header data is useless at runtime, so only bother copying
    // sections into the reserved region.

    // TODO: Load everything except .text first. Since we'll translate .text
    // on load, its size is unknown. We need known offsets to the data sections
    // while translating, which would otherwise be impossible.

    // TODO: Pass .text section buffer to an emitter, so we translate .text on
    // program load.

    fclose(nro);
    return output;
}

