#include "join_dump.h"

#include "tuple_store.h"

#include <errno.h>
#include <string.h>

int join_dump(
    const StatData *a, size_t na,
    const StatData *b, size_t nb,
    StatData **out, size_t *nout)
{
    if (out == NULL || nout == NULL) {
        return -EINVAL;
    }
    if ((na > 0 && a == NULL) || (nb > 0 && b == NULL)) {
        return -EINVAL;
    }

    *out = NULL;
    *nout = 0;

    int rc = 0;
    TupleStore store;
    memset(&store, 0, sizeof(store));

    rc = tuplestore_init(&store);
    if (rc != 0) {
        return rc;
    }

    for (size_t i = 0; i < na; ++i) {
        rc = tuplestore_append(&a[i], &store);
        if (rc != 0) {
            tuplestore_destroy(&store);
            return rc;
        }
    }

    for (size_t i = 0; i < nb; ++i) {
        rc = tuplestore_append(&b[i], &store);
        if (rc != 0) {
            tuplestore_destroy(&store);
            return rc;
        }
    }

    rc = tuplestore_sort_and_compact(&store);
    if (rc != 0) {
        tuplestore_destroy(&store);
        return rc;
    }

    *out = store.rows;
    *nout = store.len;

    store.rows = NULL;
    store.len = 0;
    store.cap = 0;

    return 0;
}