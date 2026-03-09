#include "join_dump.h"
#include "load_dump.h"
#include "print_dump.h"
#include "sort_dump.h"
#include "store_dump.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static void free_all(StatData *a, StatData *b, StatData *merged) {
    free(a);
    free(b);
    free(merged);
}

int main(int argc, char **argv) {
    int rc = 0;
    StatData *a = NULL;
    StatData *b = NULL;
    StatData *merged = NULL;
    size_t na = 0;
    size_t nb = 0;
    size_t nmerged = 0;
    
    if (argc != 4) {
        fprintf(stderr, "usage: %s input_a.bin input_b.bin output.bin\n", argv[0]);
        return 1;
    }

    rc = load_dump_file(argv[1], &a, &na);
    if (rc != 0) {
        fprintf(stderr, "load_dump_file(%s) failed: %d\n", argv[1], rc);
        free_all(a, b, merged);
        return 1;
    }

    rc = load_dump_file(argv[2], &b, &nb);
    if (rc != 0) {
        fprintf(stderr, "load_dump_file(%s) failed: %d\n", argv[2], rc);
        free_all(a, b, merged);
        return 1;
    }

    rc = join_dump(a, na, b, nb, &merged, &nmerged);
    if (rc != 0) {
        fprintf(stderr, "join_dump failed: %d\n", rc);
        free_all(a, b, merged);
        return 1;
    }

    sort_dump(merged, nmerged);

    print_dump_table(stdout, merged, nmerged, 10);

    rc = store_dump_file(argv[3], merged, nmerged);
    if (rc != 0) {
        fprintf(stderr, "store_dump_file(%s) failed: %d\n", argv[3], rc);
        free_all(a, b, merged);
        return 1;
    }

    free_all(a, b, merged);
    return 0;
}