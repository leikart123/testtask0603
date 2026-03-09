#include "tuple_store.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "log.h"
#include <string.h>
#include <unistd.h>

#define START_ROW_N 8192

static int statdata_cmp_by_id(const void *lhs, const void *rhs) {
    const StatData *a;
    const StatData *b;

    a = (const StatData *)lhs;
    b = (const StatData *)rhs;

    if (a->id < b->id) {
        return -1;
    }

    if (a->id > b->id) {
        return 1;
    }

    return 0;
}

int tuplestore_init(TupleStore *s) {
    assert(s);
    s->rows = calloc(START_ROW_N, sizeof(StatData));
    if (!s->rows) {
        fprintf(stderr, "Failed to allocate rows array in tuplestore_init %d\n", errno);
        return -errno;
    }
    s->len = 0;
    s->cap = START_ROW_N;
    return 0;
}

void tuplestore_destroy(TupleStore *s) {
    assert(s);
    free(s->rows);
    s->rows = NULL;
    s->len = 0;
    s->cap = 0;
}

int tuplestore_append(const StatData *row, TupleStore *s) {
    assert(s);
    assert(row);
    
    if (s->len == s->cap) {
        StatData *new = NULL;
        new = realloc(s->rows, s->cap * 2 * sizeof(StatData));
        if (!new) {
            fprintf(stderr, "Failed to realloc rows array in tuplestore %d", errno);
            return -errno;
        }
        s->rows = new;
        s->cap *= 2;
    }
    memcpy(&s->rows[s->len], (void *)row, sizeof(StatData));
    s->len++;
    return 0;
}

static void statdata_merge(StatData *dst, const StatData *src) {
    dst->count += src->count;
    dst->cost += src->cost;
    dst->primary = dst->primary & src->primary;

    if (src->mode > dst->mode) {
        dst->mode = src->mode;
    }
}

int tuplestore_sort_and_compact(TupleStore *s) {
    size_t write_pos;
    size_t read_pos;
    assert(s);

    if (s->len == 0) {
        return 0;
    }

    qsort(s->rows, s->len, sizeof(StatData), statdata_cmp_by_id);
    write_pos = 0;

    for (read_pos = 1; read_pos < s->len; ++read_pos) {
        if (s->rows[write_pos].id == s->rows[read_pos].id) {
            statdata_merge(&s->rows[write_pos], &s->rows[read_pos]);
        } else {
            write_pos += 1;
            s->rows[write_pos] = s->rows[read_pos];
        }
    }

    s->len = write_pos + 1;
    return 0;
}
