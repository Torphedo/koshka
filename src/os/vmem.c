#include <stdlib.h>

#include "platform.h"
#ifdef POSIX
    #include <sys/mman.h>
#elif defined(__WIN32)
    #include <Windows.h> 
#endif

#include <types.h>
#include "vmem.h"

void* userspace = NULL;
typedef struct {
    void* ptr;
    u64 size;
    vmem_permission perm;
}allocation;

allocation buffers[1024] = {0};
u32 next_buf = 0;

void* vmem_alloc(u64 size, vmem_permission perm) {
    buffers[next_buf].size = size;
    buffers[next_buf].perm = perm;

    if (perm == MEM_R || perm == MEM_RW) {
        buffers[next_buf].ptr = malloc(size);
    }
    else {
        void* ptr = NULL;
        #ifdef POSIX
            int rwx = PROT_READ | PROT_WRITE | PROT_EXEC;
            ptr = mmap(NULL, size, rwx, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        #elif defined(__WIN32)
            // TODO: Could we use an enclave here?
            DWORD rwx = PAGE_EXECUTE_READWRITE;
            ptr = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, rwx);
        #endif
        buffers[next_buf].ptr = ptr;
    }

    return buffers[next_buf++].ptr;
}

void* vmem_map(u64 size, vmem_permission perm) {
#ifdef POSIX
    int prot = 0;
    switch (perm) {
        case MEM_R:
            prot = PROT_READ;
            break;
        case MEM_RW:
            prot = PROT_READ | PROT_WRITE;
            break;
        default:
            prot = PROT_READ | PROT_WRITE | PROT_EXEC;
            break;
    };
    return mmap(NULL, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#elif defined(__WIN32)
    // TODO: Could we use an enclave here?
    DWORD prot = 0;
    switch (perm) {
        case MEM_R:
            prot = PAGE_READONLY;
            break;
        case MEM_RW:
            prot = PAGE_READWRITE;
            break;
        default:
            prot = PAGE_EXECUTE_READWRITE;
            break;
    };
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, prot);
#endif
}

allocation* alloc_by_ptr(void* ptr) {
    u32 count = sizeof(buffers) / sizeof(*buffers);
    for (int i = 0; i < count; i++) {
        if (buffers[i].ptr == ptr) {
            return &buffers[i];
        }
    }
    return NULL;
}

void vmem_free(void* ptr) {
    allocation* buf = alloc_by_ptr(ptr);
    if (buf->perm != MEM_RWX) {
        free(ptr);
    }
    #ifdef POSIX
        munmap(ptr, buf->size);
    #elif defined(__WIN32)
        VirtualFree(ptr, buf->size, MEM_DECOMMIT | MEM_RELEASE);
    #endif
}

void vmem_unmap(void* ptr, u64 size) {
    #ifdef POSIX
        munmap(ptr, size);
    #elif defined(__WIN32)
        VirtualFree(ptr, size, MEM_DECOMMIT | MEM_RELEASE);
    #endif
}

