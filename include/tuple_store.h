#pragma once

#include <stddef.h>
#include "statdata.h"

typedef struct {
    StatData *rows;
    size_t len;
    size_t cap;
} TupleStore;

int tuplestore_init(TupleStore *s);
void tuplestore_destroy(TupleStore *s);
int tuplestore_append(const StatData *row, TupleStore *s);
int tuplestore_sort_and_compact(TupleStore *s);