#include "sort_dump.h"

#include <stdlib.h>

static int cmp_by_cost_then_id(const void *lhs, const void *rhs) {
    const StatData *a = (const StatData *)lhs;
    const StatData *b = (const StatData *)rhs;

    if (a->cost < b->cost) 
        return -1;
    if (a->cost > b->cost) 
        return 1;

    if (a->id < b->id) 
        return -1;
    if (a->id > b->id) 
        return 1;

    return 0;
}

void sort_dump(StatData *rows, size_t n) {
    if (rows == NULL || n < 2) {
        return;
    }

    qsort(rows, n, sizeof(*rows), cmp_by_cost_then_id);
}