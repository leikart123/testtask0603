#include "load_dump.h"
#include "buffer_pool.h"
#include "dump.h"
#include "dump_scan.h"
#include "tuple_store.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static int tuplestore_append_cb(const StatData *row, void *arg) {
    return tuplestore_append(row, (TupleStore *)arg);
}

int load_dump_fd(int fd, StatData **rows, size_t *n) {
    if (fd < 0 || rows == NULL || n == NULL) {
        return -EINVAL;
    }

    *rows = NULL;
    *n = 0;

    int rc = 0;
    TupleStore store;
    memset(&store, 0, sizeof(store));

    rc = tuplestore_init(&store);
    if (rc != 0) {
        return rc;
    }

    BufferPool *pool = new_buffer_pool(1, MORSEL_SIZE * PAGE_SIZE, PAGE_SIZE);
    if (pool == NULL) {
        tuplestore_destroy(&store);
        return -ENOMEM;
    }

    rc = scan_dump(fd, pool, tuplestore_append_cb, &store);

    free_buffer_pool(pool);

    if (rc != 0) {
        tuplestore_destroy(&store);
        return rc;
    }

    *rows = store.rows;
    *n = store.len;

    store.rows = NULL;
    store.len = 0;
    store.cap = 0;

    return 0;
}

int load_dump_file(const char *path, StatData **rows, size_t *n) {
    if (path == NULL || rows == NULL || n == NULL) {
        return -EINVAL;
    }

    *rows = NULL;
    *n = 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -errno;
    }

    int rc = load_dump_fd(fd, rows, n);

    if (close(fd) != 0 && rc == 0) {
        rc = -errno;
    }

    return rc;
}