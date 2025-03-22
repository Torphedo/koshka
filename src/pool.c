#include "pool.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <common/logging.h>

bool pool_can_hold(pool_t pool, pool_size_t size) {
    // Pool's... open?
    return pool.alloc_size > (pool.pos + size);
}

void* pool_getdata(pool_t pool, pool_handle handle) {
    return (void*)(pool.data + handle);
}

pool_t pool_open(pool_size_t init_size) {
    return (pool_t) {
        .data = (uintptr_t)calloc(1, init_size),
        .alloc_size = init_size,
    };
}

// Open/close doesn't make much sense for a pool, frankly I just wanted an
// excuse to write "Pool's closed."
void pool_close(pool_t* pool) {
    free((void*)pool->data);
    *pool = (pool_t){0}; // Wipe our state
    // Pool's closed.
}

// TODO: Use a list for our backing buffer so we can re-use that code?
// Or merge this into bobtail and use it as the basis for the list?
pool_handle pool_push(pool_t* pool, const void* data, pool_size_t data_size, pool_size_t alloc_size) {
    if (alloc_size == 0) {
        alloc_size = data_size;
    }
    // If there's no room, we need to realloc
    if (!pool_can_hold(*pool, alloc_size)) {
        // The buffer is completely full & needs a new allocation. Grow by 50%.
        const pool_size_t newsize = (pool->alloc_size + alloc_size) * 1.5;
        assert(newsize > pool->alloc_size); // Sanity check to avoid memory corruption
        void* newbuf = calloc(1, newsize);
        if (newbuf == NULL) {
            LOG_MSG(error, "Couldn't expand 0x%X -> 0x%X [alloc failure]\n", pool->alloc_size, newsize);
            return POOL_INVALID_HANDLE;
        }

        // Copy data & update state
        memcpy(newbuf, (void*)pool->data, pool->alloc_size);
        free((void*)pool->data);
        pool->data = (uintptr_t)newbuf;
        pool->alloc_size = newsize;
    }

    // Copy the new data in & add it to the pool
    memcpy(pool_getdata(*pool, pool->pos), data, data_size);
    const pool_handle idx = pool->pos;
    pool->pos += alloc_size;

    return idx;
}

void pool_drain(pool_t* pool) {
    memset((void*)pool->data, 0x00, pool->alloc_size);
    pool->pos = 0;
}

bool pool_empty(pool_t pool) {
    return pool.pos == 0 || pool.alloc_size == 0 || pool.data == 0;
}
