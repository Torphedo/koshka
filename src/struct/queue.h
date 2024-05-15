#ifndef QUEUE_H
#define QUEUE_H

#include "../types.h"

typedef struct {
    // Base allocation & size
    u64* data;
    u32 alloc_size;

    u32 front_idx;
    u32 back_idx;
}queue;

queue queue_create(u32 init_size);
void queue_add(queue* q, u64 val);
u64 queue_get(queue* q);

void queue_clear(queue* q);

#endif // #ifndef QUEUE_H
