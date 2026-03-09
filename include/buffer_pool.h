#pragma once
#include <stdlib.h>
#include "mpmc_queue.h"

typedef struct {
    void *data;
    size_t capacity;
    uint64_t idx;
    size_t used;
}   Buffer;

typedef struct {
    mpmc_q *free_queue;
    size_t buf_size;
    size_t capacity; // upper pow2 from nbuf
    size_t nbuf; // number of buffers
    size_t arena_size;
    void *memory_arena;
    size_t align;
} BufferPool;

BufferPool *new_buffer_pool(size_t capacity, size_t buf_size, size_t align);
int  bufferpool_try_acquire(BufferPool *p, Buffer *out);
void bufferpool_release(BufferPool *p, Buffer *buf);
void    free_buffer_pool(BufferPool *pool);
  