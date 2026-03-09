#include "store_dump.h"
#include "statdata.h"

#include <stdio.h>
#include <stdlib.h>

static const StatData case_1_in_a[] = {
    {.id = 90889, .count = 13, .cost = 3.567f,   .primary = 0, .mode = 3},
    {.id = 90089, .count = 1,  .cost = 88.90f,   .primary = 1, .mode = 0},
};

static const StatData case_1_in_b[] = {
    {.id = 90089, .count = 13,   .cost = 0.011f,   .primary = 0, .mode = 2},
    {.id = 90189, .count = 1000, .cost = 1.00003f, .primary = 1, .mode = 2},
};

static const StatData case_1_out[] = {
    {.id = 90189, .count = 1000, .cost = 1.00003f, .primary = 1, .mode = 2},
    {.id = 90889, .count = 13,   .cost = 3.567f,   .primary = 0, .mode = 3},
    {.id = 90089, .count = 14,   .cost = 88.911f,  .primary = 0, .mode = 2},
};

int main(void) {
    int rc = 0;

    rc = store_dump_file("input_a.bin",
                         case_1_in_a,
                         sizeof(case_1_in_a) / sizeof(case_1_in_a[0]));
    if (rc != 0) {
        fprintf(stderr, "store_dump_file(input_a.bin) failed: %d\n", rc);
        return 1;
    }

    rc = store_dump_file("input_b.bin",
                         case_1_in_b,
                         sizeof(case_1_in_b) / sizeof(case_1_in_b[0]));
    if (rc != 0) {
        fprintf(stderr, "store_dump_file(input_b.bin) failed: %d\n", rc);
        return 1;
    }

    rc = store_dump_file("expected.bin",
                         case_1_out,
                         sizeof(case_1_out) / sizeof(case_1_out[0]));
    if (rc != 0) {
        fprintf(stderr, "store_dump_file(expected.bin) failed: %d\n", rc);
        return 1;
    }

    printf("generated input_a.bin, input_b.bin, expected.bin\n");
    return 0;
}