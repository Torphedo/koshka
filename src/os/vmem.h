#ifndef VMEM_H
#define VMEM_H

/* vmem.h: Helper functions for handling the emulated "userspace". */
#include <common/int.h>

typedef enum {
    MEM_R,
    MEM_RW,
    MEM_RWX
}vmem_permission;

void* vmem_alloc(u64 size, vmem_permission perm);
void* vmem_map(u64 size, vmem_permission perm);

void vmem_free(void* ptr);
void vmem_unmap(void* ptr, u64 size);

#endif // #ifndef VMEM_H

