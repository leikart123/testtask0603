#include "mpmc_queue.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static int is_pow2(size_t x) {
    return x && ((x & (x - 1)) == 0);
}

int mpmc_init(mpmc_q* q, size_t capacity_pow2)
{
    if (!q) 
       return -EINVAL;
    if (capacity_pow2 < 2 || !is_pow2(capacity_pow2)) 
        return -EINVAL;

    memset(q, 0, sizeof(*q));

    q->cap  = capacity_pow2;
    q->mask = capacity_pow2 - 1;

    q->buf = (mpmc_cell*)calloc(q->cap, sizeof(mpmc_cell));
    if (!q->buf) 
        return -ENOMEM;

    for (size_t i = 0; i < q->cap; ++i) {
        atomic_init(&q->buf[i].seq, i);
        q->buf[i].data = 0;
    }

    atomic_init(&q->enq_pos, 0);
    atomic_init(&q->deq_pos, 0);
    return 0;
}

void mpmc_destroy(mpmc_q* q)
{
    if (!q) 
        return;
    free(q->buf);
    q->buf = NULL;
    q->cap = 0;
    q->mask = 0;
}

size_t mpmc_capacity(const mpmc_q* q)
{
    return q ? q->cap : 0;
}

int mpmc_try_enq(mpmc_q* q, uint32_t v)
{
    mpmc_cell* cell;
    size_t pos = atomic_load_explicit(&q->enq_pos, memory_order_relaxed);

    for (;;) {
        cell = &q->buf[pos & q->mask];
        size_t seq = atomic_load_explicit(&cell->seq, memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)pos;

        if (dif == 0) {
            size_t expected = pos;
            if (atomic_compare_exchange_weak_explicit(
                    &q->enq_pos, &expected, pos + 1,
                    memory_order_relaxed, memory_order_relaxed))
            {
                break;
            }
            pos = expected;
            continue;
        }

        if (dif < 0) {
            return -EAGAIN;
        }

        pos = atomic_load_explicit(&q->enq_pos, memory_order_relaxed);
    }

    cell->data = v;
    atomic_store_explicit(&cell->seq, pos + 1, memory_order_release);
    return 0;
}

int mpmc_try_deq(mpmc_q* q, uint32_t* out)
{
    mpmc_cell* cell;
    size_t pos = atomic_load_explicit(&q->deq_pos, memory_order_relaxed);

    for (;;) {
        cell = &q->buf[pos & q->mask];
        size_t seq = atomic_load_explicit(&cell->seq, memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

        if (dif == 0) {
            size_t expected = pos;
            if (atomic_compare_exchange_weak_explicit(
                    &q->deq_pos, &expected, pos + 1,
                    memory_order_relaxed, memory_order_relaxed))
            {
                break;
            }
            pos = expected;
            continue;
        }

        if (dif < 0) {
            return -EAGAIN;
        }

        pos = atomic_load_explicit(&q->deq_pos, memory_order_relaxed);
    }

    uint32_t v = cell->data;
    if (out) *out = v;

    atomic_store_explicit(&cell->seq, pos + q->cap, memory_order_release);
    return 0;
}