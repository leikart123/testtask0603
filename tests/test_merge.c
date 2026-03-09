#include "test_common.h"

#include <stdio.h>

static const StatData empty_rows[] = {
};

const StatData case_1_in_a[2] =
{{.id = 90889, .count = 13, .cost = 3.567, .primary = 0, .mode=3 },
{.id = 90089, .count = 1, .cost = 88.90, .primary = 1, .mode=0 }};

const StatData case_1_in_b[2] =
{{.id = 90089, .count = 13, .cost = 0.011, .primary = 0, .mode=2 },
{.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode=2}};

/* Ожидаемый результат обработки */
const StatData case_1_out[3] =
{{.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode = 2 },
{.id = 90889, .count = 13, .cost = 3.567, .primary = 0, .mode = 3 },
{.id = 90089, .count = 14, .cost = 88.911, .primary = 0, .mode = 2 }};

static const StatData dup_inside_a_a[] = {
    { .id = 42, .count = 1, .cost = 1.0f, .primary = 1, .mode = 1 },
    { .id = 42, .count = 2, .cost = 2.5f, .primary = 1, .mode = 3 },
};

static const StatData dup_inside_a_b[] = {
    { .id = 7, .count = 9, .cost = 9.0f, .primary = 1, .mode = 1 },
};

static const StatData dup_inside_a_expected[] = {
    { .id = 42, .count = 3, .cost = 3.5f, .primary = 1, .mode = 3 },
    { .id = 7, .count = 9, .cost = 9.0f, .primary = 1, .mode = 1 },
};

static const StatData dup_both_a[] = {
    { .id = 10, .count = 1, .cost = 1.0f, .primary = 1, .mode = 1 },
    { .id = 20, .count = 2, .cost = 8.0f, .primary = 1, .mode = 2 },
};

static const StatData dup_both_b[] = {
    { .id = 10, .count = 4, .cost = 2.0f, .primary = 0, .mode = 3 },
    { .id = 30, .count = 1, .cost = 0.5f, .primary = 1, .mode = 1 },
};

static const StatData dup_both_expected[] = {
    { .id = 30, .count = 1, .cost = 0.5f, .primary = 1, .mode = 1 },
    { .id = 10, .count = 5, .cost = 3.0f, .primary = 0, .mode = 3 },
    { .id = 20, .count = 2, .cost = 8.0f, .primary = 1, .mode = 2 },
};

static const StatData primary_mode_a[] = {
    { .id = 100, .count = 1, .cost = 5.0f, .primary = 1, .mode = 1 },
};

static const StatData primary_mode_b[] = {
    { .id = 100, .count = 2, .cost = 6.0f, .primary = 0, .mode = 7 },
};

static const StatData primary_mode_expected[] = {
    { .id = 100, .count = 3, .cost = 11.0f, .primary = 0, .mode = 7 },
};

static const StatData cost_sort_a[] = {
    { .id = 1, .count = 1, .cost = 10.0f, .primary = 1, .mode = 1 },
    { .id = 2, .count = 1, .cost = 3.0f, .primary = 1, .mode = 1 },
};

static const StatData cost_sort_b[] = {
    { .id = 3, .count = 1, .cost = 7.0f, .primary = 1, .mode = 1 },
};

static const StatData cost_sort_expected[] = {
    { .id = 2, .count = 1, .cost = 3.0f, .primary = 1, .mode = 1 },
    { .id = 3, .count = 1, .cost = 7.0f, .primary = 1, .mode = 1 },
    { .id = 1, .count = 1, .cost = 10.0f, .primary = 1, .mode = 1 },
};

int run_merge_tests(void)
{
    const TestCase tests[] = {
        {
            .name = "basic_merge_sort",
            .in_a = case_1_in_a,
            .in_a_n = sizeof(case_1_in_a) / sizeof(case_1_in_a[0]),
            .in_b = case_1_in_b,
            .in_b_n = sizeof(case_1_in_b) / sizeof(case_1_in_b[0]),
            .expected = case_1_out,
            .expected_n = sizeof(case_1_out) / sizeof(case_1_out[0]),
        },
        {
            .name = "empty_empty",
            .in_a = empty_rows,
            .in_a_n = 0,
            .in_b = empty_rows,
            .in_b_n = 0,
            .expected = empty_rows,
            .expected_n = 0,
        },
        {
            .name = "dup_inside_a",
            .in_a = dup_inside_a_a,
            .in_a_n = sizeof(dup_inside_a_a) / sizeof(dup_inside_a_a[0]),
            .in_b = dup_inside_a_b,
            .in_b_n = sizeof(dup_inside_a_b) / sizeof(dup_inside_a_b[0]),
            .expected = dup_inside_a_expected,
            .expected_n = sizeof(dup_inside_a_expected) / sizeof(dup_inside_a_expected[0]),
        },
        {
            .name = "dup_in_both_inputs",
            .in_a = dup_both_a,
            .in_a_n = sizeof(dup_both_a) / sizeof(dup_both_a[0]),
            .in_b = dup_both_b,
            .in_b_n = sizeof(dup_both_b) / sizeof(dup_both_b[0]),
            .expected = dup_both_expected,
            .expected_n = sizeof(dup_both_expected) / sizeof(dup_both_expected[0]),
        },
        {
            .name = "primary_and_mode_rules",
            .in_a = primary_mode_a,
            .in_a_n = sizeof(primary_mode_a) / sizeof(primary_mode_a[0]),
            .in_b = primary_mode_b,
            .in_b_n = sizeof(primary_mode_b) / sizeof(primary_mode_b[0]),
            .expected = primary_mode_expected,
            .expected_n = sizeof(primary_mode_expected) / sizeof(primary_mode_expected[0]),
        },
        {
            .name = "result_sorted_by_cost",
            .in_a = cost_sort_a,
            .in_a_n = sizeof(cost_sort_a) / sizeof(cost_sort_a[0]),
            .in_b = cost_sort_b,
            .in_b_n = sizeof(cost_sort_b) / sizeof(cost_sort_b[0]),
            .expected = cost_sort_expected,
            .expected_n = sizeof(cost_sort_expected) / sizeof(cost_sort_expected[0]),
        },
    };
    size_t i;
    size_t total;
    size_t passed;
    int rc;

    total = 0;
    passed = 0;

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        total += 1;
        rc = run_one_test(&tests[i]);
        if (rc == 0) {
            passed += 1;
        }
    }

    printf("merge tests: %zu/%zu passed\n", passed, total);

    if (passed != total) {
        return 1;
    }

    return 0;
}