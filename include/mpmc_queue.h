#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
// Dmitry Vyukov's bounded MPMC queue algorithm
typedef struct {
    atomic_size_t seq;
    uint32_t      data;
} mpmc_cell;

typedef struct  {
    mpmc_cell*    buf;
    size_t        mask;
    size_t        cap;
    atomic_size_t enq_pos;
    atomic_size_t deq_pos;
} mpmc_q;
// capacity_pow2: обязательно степень двойки
// Возврат: 0 ok, -EINVAL если capacity не pow2/маленькая, -ENOMEM если не выделилась память
int  mpmc_init(mpmc_q* q, size_t capacity_pow2);
void mpmc_destroy(mpmc_q* q);

// 0 ok, -EAGAIN если очередь полна
int  mpmc_try_enq(mpmc_q* q, uint32_t v);

// 0 ok, -EAGAIN если очередь пуста
int  mpmc_try_deq(mpmc_q* q, uint32_t* out);

size_t mpmc_capacity(const mpmc_q* q);