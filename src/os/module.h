#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <common/int.h>

typedef struct {
    u8* base;
    u8* entry_point;
}module;

// See https://switchbrew.org/wiki/NSO#MOD
typedef struct {
    u32 reserved;
    u32 magic_offset; // Always 8
    u32 magic; // "MOD0"
    u32 dynamic_seg_offset;
    u32 bss_seg_offset;
    u32 bss_start;
    u32 bss_end;
    u32 eh_frame_hdr_start;
    u32 eh_frame_hdr_end;
    u32 mod_obj_offset; // ??? (apparently usually == bss base)?
}mod0;


#ifdef __cplusplus
}
#endif
