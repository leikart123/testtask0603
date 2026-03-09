#include "test_common.h"
#include "load_dump.h"
#include "store_dump.h"

#include <errno.h>
#include <math.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int compare_stat(const StatData *a, const StatData *b)
{
    if (a->id != b->id) {
        return 0;
    }

    if (a->count != b->count) {
        return 0;
    }

    if (fabsf(a->cost - b->cost) > 1e-6f) {
        return 0;
    }

    if (a->primary != b->primary) {
        return 0;
    }

    if (a->mode != b->mode) {
        return 0;
    }

    return 1;
}

int compare_rows(
    const StatData *got,
    size_t got_n,
    const StatData *exp,
    size_t exp_n,
    const char *case_name)
{
    size_t i;

    if (got_n != exp_n) {
        fprintf(stderr, "[%s] size mismatch: got=%zu expected=%zu\n",
            case_name, got_n, exp_n);
        return -1;
    }

    for (i = 0; i < got_n; ++i) {
        if (!compare_stat(&got[i], &exp[i])) {
            fprintf(stderr,
                "[%s] row %zu mismatch:\n"
                " got: id=%ld count=%d cost=%f primary=%u mode=%u\n"
                " exp: id=%ld count=%d cost=%f primary=%u mode=%u\n",
                case_name,
                i,
                got[i].id,
                got[i].count,
                got[i].cost,
                got[i].primary,
                got[i].mode,
                exp[i].id,
                exp[i].count,
                exp[i].cost,
                exp[i].primary,
                exp[i].mode);
            return -1;
        }
    }

    return 0;
}

static int write_case_inputs(
    const TestCase *tc,
    const char *path_a,
    const char *path_b)
{
    int rc;

    rc = store_dump_file(path_a, tc->in_a, tc->in_a_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] store_dump_file(%s) failed: %d\n",
            tc->name, path_a, rc);
        return rc;
    }

    rc = store_dump_file(path_b, tc->in_b, tc->in_b_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] store_dump_file(%s) failed: %d\n",
            tc->name, path_b, rc);
        return rc;
    }

    return 0;
}

int run_statmerge(
    const char *path_a,
    const char *path_b,
    const char *path_out)
{
    pid_t pid;
    int rc;
    int status;
    char *const argv[] = {
        "./bin/statmerge",
        (char *)path_a,
        (char *)path_b,
        (char *)path_out,
        NULL
    };

    status = 0;

    rc = posix_spawn(&pid, "./bin/statmerge", NULL, NULL, argv, environ);
    if (rc != 0) {
        return -rc;
    }

    rc = waitpid(pid, &status, 0);
    if (rc < 0) {
        return -errno;
    }

    if (!WIFEXITED(status)) {
        return -ECHILD;
    }

    if (WEXITSTATUS(status) != 0) {
        return -WEXITSTATUS(status);
    }

    return 0;
}

int run_one_test(const TestCase *tc)
{
    char path_a[128];
    char path_b[128];
    char path_out[128];
    StatData *got;
    size_t got_n;
    int rc;

    got = NULL;
    got_n = 0;

    snprintf(path_a, sizeof(path_a), "/tmp/%s_a.bin", tc->name);
    snprintf(path_b, sizeof(path_b), "/tmp/%s_b.bin", tc->name);
    snprintf(path_out, sizeof(path_out), "/tmp/%s_out.bin", tc->name);

    rc = write_case_inputs(tc, path_a, path_b);
    if (rc != 0) {
        unlink(path_a);
        unlink(path_b);
        unlink(path_out);
        return rc;
    }

    rc = run_statmerge(path_a, path_b, path_out);
    if (rc != 0) {
        fprintf(stderr, "[%s] statmerge failed: %d\n", tc->name, rc);
        unlink(path_a);
        unlink(path_b);
        unlink(path_out);
        return rc;
    }

    rc = load_dump_file(path_out, &got, &got_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] load_dump_file(%s) failed: %d\n",
            tc->name, path_out, rc);
        unlink(path_a);
        unlink(path_b);
        unlink(path_out);
        return rc;
    }

    rc = compare_rows(got, got_n, tc->expected, tc->expected_n, tc->name);
    if (rc != 0) {
        free(got);
        unlink(path_a);
        unlink(path_b);
        unlink(path_out);
        return rc;
    }

    printf("[OK] %s\n", tc->name);

    free(got);
    unlink(path_a);
    unlink(path_b);
    unlink(path_out);

    return 0;
}

int run_dump_roundtrip_test(
    const char *name,
    const StatData *rows,
    size_t rows_n)
{
    char path[128];
    StatData *got;
    size_t got_n;
    int rc;

    got = NULL;
    got_n = 0;

    snprintf(path, sizeof(path), "/tmp/%s_dump.bin", name);

    rc = store_dump_file(path, rows, rows_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] store_dump_file failed: %d\n", name, rc);
        unlink(path);
        return rc;
    }

    rc = load_dump_file(path, &got, &got_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] load_dump_file failed: %d\n", name, rc);
        unlink(path);
        return rc;
    }

    rc = compare_rows(got, got_n, rows, rows_n, name);
    if (rc != 0) {
        free(got);
        unlink(path);
        return rc;
    }

    printf("[OK] %s\n", name);

    free(got);
    unlink(path);

    return 0;
}