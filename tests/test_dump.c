#include "test_common.h"

#include <stdio.h>

static const StatData empty_rows[] = {
};

static const StatData dump_single[] = {
    { .id = 123, .count = 5, .cost = 42.5f, .primary = 1, .mode = 6 },
};

static const StatData dump_many[] = {
    { .id = 1, .count = 1, .cost = 1.0f, .primary = 1, .mode = 1 },
    { .id = 2, .count = 2, .cost = 2.0f, .primary = 0, .mode = 2 },
    { .id = 3, .count = 3, .cost = 3.0f, .primary = 1, .mode = 3 },
    { .id = 4, .count = 4, .cost = 4.0f, .primary = 0, .mode = 4 },
    { .id = 5, .count = 5, .cost = 5.0f, .primary = 1, .mode = 5 },
    { .id = 6, .count = 6, .cost = 6.0f, .primary = 0, .mode = 6 },
};

int run_dump_tests(void)
{
    size_t total;
    size_t passed;
    int rc;

    total = 0;
    passed = 0;

    total += 1;
    rc = run_dump_roundtrip_test("dump_roundtrip_empty", empty_rows, 0);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_roundtrip_test(
        "dump_roundtrip_single",
        dump_single,
        sizeof(dump_single) / sizeof(dump_single[0]));
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_roundtrip_test(
        "dump_roundtrip_many",
        dump_many,
        sizeof(dump_many) / sizeof(dump_many[0]));
    if (rc == 0) {
        passed += 1;
    }

    printf("dump tests: %zu/%zu passed\n", passed, total);

    if (passed != total) {
        return 1;
    }

    return 0;
}