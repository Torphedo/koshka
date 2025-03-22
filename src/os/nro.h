#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <common/int.h>
#include "module.h"

// Struct information sourced from https://switchbrew.org/wiki/NRO
typedef struct {
    u32 unused;
    u32 mod0_offset;
    u64 pad;
}nro_start;

typedef struct {
    u32 offset;
    u32 size;
}nro_seghdr;

typedef struct {
     u8 val[0x20];
}gnu_build_id;

typedef struct {
    u32 magic; // "NRO0"
    u32 version; // Always 0
    u32 size;
    u32 flags;
    nro_seghdr text_seg;
    nro_seghdr ro_seg;
    nro_seghdr data_seg;
    u32 bss_size;
    u32 reserved;
    gnu_build_id id;
    u32 dso_handle_offset;
    u32 reserved2;
    nro_seghdr api_info_seg;
    nro_seghdr dynstr_seg;
    nro_seghdr dynsym_seg;
}nro_hdr;

typedef struct {
    u64 offset;
    u64 size;
}nro_asset_section;

// Located at nro_hdr.size
typedef struct {
    u32 magic; // "ASET"
    u32 version; // 0
    nro_asset_section icon; // 256^2 JPEG
    nro_asset_section nacp;
    nro_asset_section romfs;
}nro_asset_hdr;

module nro_load(const char* path);

#ifdef __cplusplus
}
#endif
