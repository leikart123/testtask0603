#include "mpmc_queue.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include "buffer_pool.h"

void bufferpool_release(BufferPool *p, Buffer *buf) {
    assert(p);
    assert(buf);
    int rc = 0;
    rc = mpmc_try_enq(p->free_queue, buf->idx);
    assert(rc == 0);
}

int  bufferpool_try_acquire(BufferPool *p, Buffer *out) {
    assert(p);
    assert(out);
    uint32_t idx = 0;
    int rc = 0;
    out->capacity = p->buf_size;
    out->used = 0;
    rc = mpmc_try_deq(p->free_queue, &idx);
    if (rc == -EAGAIN)
        return rc;
    assert(idx < p->nbuf);
    out->idx = idx;
    uint8_t *base = (uint8_t*)p->memory_arena;
    out->data = base + (size_t)idx * p->buf_size;
/* #ifdef DEBUG
    printf("[DEBUG] Buffer:\n");
    printf("  data: %p\n", out->data);
    printf("  size: %zu\n", out->size);
    printf("  idx : %" PRIu64 "\n", out->idx);
    printf("  len : %zu\n", out->len);
#endif */
    return 0;
}

static inline uint64_t ceil_pow2_u64(uint64_t x) {
    if (x <= 1) return 2;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

void    free_buffer_pool(BufferPool *pool) {
    assert(pool->free_queue != NULL);
    assert(pool->memory_arena != NULL);
    mpmc_destroy(pool->free_queue);
    free(pool->memory_arena);
    free(pool->free_queue);
    free(pool);
}

BufferPool *new_buffer_pool(size_t capacity, size_t buf_size, size_t align) {
    BufferPool *pool = NULL;
    int err = 0;

    assert(capacity > 0);
    assert(buf_size > 0);
    assert((align & (align - 1)) == 0);
    assert(buf_size % align == 0);
    pool = calloc(1, sizeof(BufferPool));
    if (!pool) {
        fprintf(stderr, "Failed to allocate Buffer Pool errno %d", errno);
    }
    pool->free_queue = calloc(1, sizeof(mpmc_q));
    pool->arena_size = capacity * buf_size;
    assert(pool->arena_size % align == 0);
    err = posix_memalign(&pool->memory_arena, align, pool->arena_size);
    if (err != 0) {
        fprintf(stderr, "posix_memalign failed: %d\n", err);
        free(pool->memory_arena);
        free(pool);
        return NULL;
    }
    pool->nbuf = capacity;
    pool->capacity = ceil_pow2_u64(pool->nbuf);
    pool->align = align;
    pool->buf_size = buf_size;
    err = mpmc_init(pool->free_queue, pool->capacity);
    if (err != 0)
    {
        fprintf(stderr, "Failed to allocate mpmc_queue errno %d \n", err);
        free(pool->memory_arena);
        free(pool->free_queue);
        free(pool);
        return NULL;
    }
    for (size_t i = 0; i < pool->nbuf; i++) {
        int rc = mpmc_try_enq(pool->free_queue, i);
        assert(rc == 0);
    }
#ifdef DEBUG
    fprintf(stdout, "Allocated Buffer Pool: \n \
                    buf size: %lu \n \
                    capacity: %lu \n \
                    arena_size: %lu \n \
                    align: %lu \n \
                    mpmc free queue capacity %lu \n \
                    buffer len %lu \n", pool->buf_size, pool->capacity, 
                    pool->arena_size, pool->align, pool->free_queue->cap, pool->nbuf);
#endif
    
    return pool;
}