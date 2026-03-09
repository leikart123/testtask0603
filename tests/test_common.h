#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stddef.h>
#include "statdata.h"

typedef struct TestCase {
    const char *name;
    const StatData *in_a;
    size_t in_a_n;
    const StatData *in_b;
    size_t in_b_n;
    const StatData *expected;
    size_t expected_n;
} TestCase;

int compare_rows(
    const StatData *got,
    size_t got_n,
    const StatData *exp,
    size_t exp_n,
    const char *case_name);

int run_statmerge(
    const char *path_a,
    const char *path_b,
    const char *path_out);

int run_one_test(const TestCase *tc);

int run_dump_roundtrip_test(
    const char *name,
    const StatData *rows,
    size_t rows_n);

int run_merge_tests(void);
int run_dump_tests(void);

#endif