#pragma once
#include <stdbool.h>
#include <common/int.h>

// The underlying integer size for a pool
typedef u32 pool_size_t;
// We base the handle size on this size so they always have the same integer limits.

// A stable opaque handle to data in the pool. Stays valid when the pool resizes.
typedef pool_size_t pool_handle;


/// @brief An automatically expanding "pool" of memory with stable opaque handles
typedef struct {
    /// @brief Backing buffer
    ///
    /// We use a uintptr_t so we can have a typeless pointer that can't
    /// accidentally be dereferenced.
    uintptr_t data;

    /// Current backing buffer size
    pool_size_t alloc_size;

    /// @brief Current position in backing buffer
    pool_size_t pos;
}pool_t;

enum {
    // A special handle value indicating failure
    POOL_INVALID_HANDLE = ~((pool_handle)0),
    // This will always be the largest possible pool address.
    // We use this sort of roundabout definition so it stays correct if the
    // underlying type changes.
};

/// Create a pool.
/// @param init_size The allocation size in bytes.
///
/// @return A newly initialized dynamic pool.
/// @note This allocates memory!
/// @sa pool_close
pool_t pool_open(pool_size_t init_size);

/// @brief Free pool data & fill all fields/buffer(s) with 0
///
/// @param pool Pool to destroy
/// @sa pool_open
void pool_close(pool_t* pool);

/// @brief Store data in the pool.
/// @param pool The pool to modify
/// @param data The data to store. Must be at least [data_size] bytes
/// @param data_size The number of bytes to be copied from [data]
/// @param alloc_size The number of bytes to allocate in the pool. Defaults to
/// @ref data_size if 0.
///
/// @return A stable opaque handle to the data now stored in the pool (or
/// POOL_INVALID_HANDLE on error). This is like a pointer, but still valid when
/// the pool resizes. Use @ref pool_getdata() to get a temporary pointer to the
/// data (check the function documentation for relevant warnings).
///
/// @note This allocates memory if the pool is currently full.
/// @sa pool_open()
pool_handle pool_push(pool_t* pool, const void* data, pool_size_t data_size, pool_size_t alloc_size);

/// @brief Retrieve a temporary pointer to some data stored in the pool
///
/// @param The pool to look up the handle in
/// @param The pool handle obtained from @ref pool_push()
/// @return A temporary pointer to the data referenced by the handle. The
/// pointer may become invalid over time, so don't store it anywhere and use it
/// for as little time as possible.
void* pool_getdata(pool_t pool, pool_handle handle);

/// Reset pool to initial state, fill buffer with 0. Does not free buffer.
void pool_drain(pool_t* pool);

/// @brief Whether the pool is empty
bool pool_empty(pool_t pool);

/// @brief Whether the pool has space to store data of some size
bool pool_can_hold(pool_t pool, pool_size_t size);
