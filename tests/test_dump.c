#include "test_common.h"
#include "dump.h"
#include "load_dump.h"
#include "store_dump.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

static int compare_stat_local(const StatData *a, const StatData *b)
{
    if (a->id != b->id) {
        return 0;
    }
    if (a->count != b->count) {
        return 0;
    }
    if (a->cost != b->cost) {
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

static uint64_t ceil_div_u64(uint64_t n, uint64_t d)
{
    if (d == 0) {
        return 0;
    }
    return (n + d - 1u) / d;
}

static uint32_t align_up_u32(uint32_t x, uint32_t a)
{
    return (x + a - 1u) & ~(a - 1u);
}

static int pread_full(int fd, void *buf, size_t len, off_t off)
{
    uint8_t *p;
    size_t done;

    if (fd < 0 || buf == NULL) {
        return -EINVAL;
    }

    p = (uint8_t *)buf;
    done = 0;
    while (done < len) {
        ssize_t n = pread(fd, p + done, len - done, off + (off_t)done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -errno;
        }
        if (n == 0) {
            return -EIO;
        }
        done += (size_t)n;
    }
    return 0;
}

static int pwrite_full(int fd, const void *buf, size_t len, off_t off)
{
    const uint8_t *p;
    size_t done;

    if (fd < 0 || buf == NULL) {
        return -EINVAL;
    }

    p = (const uint8_t *)buf;
    done = 0;
    while (done < len) {
        ssize_t n = pwrite(fd, p + done, len - done, off + (off_t)done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -errno;
        }
        if (n == 0) {
            return -EIO;
        }
        done += (size_t)n;
    }
    return 0;
}

static int assert_zero_region(
    const char *case_name,
    const uint8_t *page,
    size_t off,
    size_t len,
    const char *label)
{
    size_t i;

    for (i = 0; i < len; ++i) {
        if (page[off + i] != 0) {
            fprintf(stderr,
                "[%s] non-zero padding in %s at byte %zu (abs_off=%zu): %u\n",
                case_name,
                label,
                i,
                off + i,
                (unsigned)page[off + i]);
            return -1;
        }
    }
    return 0;
}

static StatData *make_rows(size_t n)
{
    StatData *rows;
    size_t i;

    if (n == 0) {
        return NULL;
    }

    rows = (StatData *)calloc(n, sizeof(*rows));
    if (rows == NULL) {
        return NULL;
    }

    for (i = 0; i < n; ++i) {
        rows[i].id = 100000L + (long)i;
        rows[i].count = (int)((i * 7u) % 1000u);
        rows[i].cost = (float)(i * 0.25f + 1.0f);
        rows[i].primary = (unsigned int)(i & 1u);
        rows[i].mode = (unsigned int)(i & 7u);
    }

    return rows;
}

static int verify_dump_layout(
    const char *case_name,
    const char *path,
    const StatData *expected_rows,
    size_t expected_n)
{
    int fd;
    int rc;
    struct stat st;
    uint8_t hdr_page[PAGE_SIZE];
    DumpFileHdr hdr;
    uint64_t expected_pages;
    uint32_t expected_data_off;
    uint32_t expected_rec_per_page;
    size_t cursor;

    fd = -1;
    rc = -1;
    memset(&st, 0, sizeof(st));
    memset(hdr_page, 0, sizeof(hdr_page));
    memset(&hdr, 0, sizeof(hdr));

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[%s] open(%s) failed: %d\n", case_name, path, errno);
        return -errno;
    }

    if (fstat(fd, &st) != 0) {
        fprintf(stderr, "[%s] fstat(%s) failed: %d\n", case_name, path, errno);
        rc = -errno;
        goto out;
    }

    rc = pread_full(fd, hdr_page, sizeof(hdr_page), 0);
    if (rc != 0) {
        fprintf(stderr, "[%s] failed to read header page: %d\n", case_name, rc);
        goto out;
    }

    memcpy(&hdr, hdr_page, sizeof(hdr));

    rc = validate_dump_hdr(&hdr);
    if (rc != 0) {
        fprintf(stderr, "[%s] validate_dump_hdr failed: %d\n", case_name, rc);
        goto out;
    }

    expected_data_off = align_up_u32(
        (uint32_t)sizeof(DumpPageHdr),
        (uint32_t)_Alignof(StatData));
    expected_rec_per_page =
        (uint32_t)((PAGE_SIZE - expected_data_off) / sizeof(StatData));
    expected_pages = ceil_div_u64((uint64_t)expected_n, (uint64_t)expected_rec_per_page);

    if (hdr.page_size != PAGE_SIZE) {
        fprintf(stderr, "[%s] bad hdr.page_size: got=%llu expected=%u\n",
            case_name,
            (unsigned long long)hdr.page_size,
            PAGE_SIZE);
        rc = -1;
        goto out;
    }

    if (hdr.sizeof_statdata != sizeof(StatData)) {
        fprintf(stderr, "[%s] bad hdr.sizeof_statdata: got=%u expected=%zu\n",
            case_name,
            hdr.sizeof_statdata,
            sizeof(StatData));
        rc = -1;
        goto out;
    }

    if (hdr.alignof_statdata != _Alignof(StatData)) {
        fprintf(stderr, "[%s] bad hdr.alignof_statdata: got=%u expected=%zu\n",
            case_name,
            hdr.alignof_statdata,
            (size_t)_Alignof(StatData));
        rc = -1;
        goto out;
    }

    if (hdr.total_records != (uint64_t)expected_n) {
        fprintf(stderr, "[%s] bad hdr.total_records: got=%llu expected=%zu\n",
            case_name,
            (unsigned long long)hdr.total_records,
            expected_n);
        rc = -1;
        goto out;
    }

    if (hdr.data_off != expected_data_off) {
        fprintf(stderr, "[%s] bad hdr.data_off: got=%u expected=%u\n",
            case_name,
            hdr.data_off,
            expected_data_off);
        rc = -1;
        goto out;
    }

    if (hdr.rec_per_page != expected_rec_per_page) {
        fprintf(stderr, "[%s] bad hdr.rec_per_page: got=%u expected=%u\n",
            case_name,
            hdr.rec_per_page,
            expected_rec_per_page);
        rc = -1;
        goto out;
    }

    if (hdr.data_pages != expected_pages) {
        fprintf(stderr, "[%s] bad hdr.data_pages: got=%llu expected=%llu\n",
            case_name,
            (unsigned long long)hdr.data_pages,
            (unsigned long long)expected_pages);
        rc = -1;
        goto out;
    }

    if ((uint64_t)st.st_size != (1u + hdr.data_pages) * (uint64_t)PAGE_SIZE) {
        fprintf(stderr,
            "[%s] bad file size: got=%llu expected=%llu\n",
            case_name,
            (unsigned long long)st.st_size,
            (unsigned long long)((1u + hdr.data_pages) * (uint64_t)PAGE_SIZE));
        rc = -1;
        goto out;
    }

    rc = assert_zero_region(
        case_name,
        hdr_page,
        sizeof(DumpFileHdr),
        PAGE_SIZE - sizeof(DumpFileHdr),
        "header-page tail");
    if (rc != 0) {
        goto out;
    }

    cursor = 0;
    for (uint32_t pno = 0; pno < hdr.data_pages; ++pno) {
        uint8_t page[PAGE_SIZE];
        DumpPageHdr ph;
        uint32_t expected_nrec;
        size_t left;
        size_t i;
        size_t used;

        memset(page, 0, sizeof(page));
        memset(&ph, 0, sizeof(ph));

        rc = pread_full(fd, page, sizeof(page), (off_t)(1u + pno) * PAGE_SIZE);
        if (rc != 0) {
            fprintf(stderr, "[%s] failed to read data page %u: %d\n",
                case_name,
                pno,
                rc);
            goto out;
        }

        memcpy(&ph, page, sizeof(ph));

        if (ph.page_no != pno) {
            fprintf(stderr, "[%s] bad page_no on page %u: got=%u expected=%u\n",
                case_name,
                pno,
                ph.page_no,
                pno);
            rc = -1;
            goto out;
        }

        left = expected_n - cursor;
        expected_nrec = (uint32_t)(
            left < (size_t)hdr.rec_per_page ? left : (size_t)hdr.rec_per_page);

        if (ph.nrec != expected_nrec) {
            fprintf(stderr, "[%s] bad nrec on page %u: got=%u expected=%u\n",
                case_name,
                pno,
                ph.nrec,
                expected_nrec);
            rc = -1;
            goto out;
        }

        rc = assert_zero_region(
            case_name,
            page,
            sizeof(DumpPageHdr),
            hdr.data_off - sizeof(DumpPageHdr),
            "page header padding");
        if (rc != 0) {
            goto out;
        }

        for (i = 0; i < ph.nrec; ++i) {
            StatData got_row;
            const uint8_t *row_ptr;

            memset(&got_row, 0, sizeof(got_row));
            row_ptr = page + hdr.data_off + i * sizeof(StatData);
            memcpy(&got_row, row_ptr, sizeof(got_row));

            if (!compare_stat_local(&got_row, &expected_rows[cursor + i])) {
                fprintf(stderr,
                    "[%s] row mismatch on page=%u row=%zu: "
                    "got{id=%ld count=%d cost=%f primary=%u mode=%u} "
                    "exp{id=%ld count=%d cost=%f primary=%u mode=%u}\n",
                    case_name,
                    pno,
                    i,
                    got_row.id,
                    got_row.count,
                    got_row.cost,
                    got_row.primary,
                    got_row.mode,
                    expected_rows[cursor + i].id,
                    expected_rows[cursor + i].count,
                    expected_rows[cursor + i].cost,
                    expected_rows[cursor + i].primary,
                    expected_rows[cursor + i].mode);
                rc = -1;
                goto out;
            }
        }

        used = (size_t)hdr.data_off + (size_t)ph.nrec * sizeof(StatData);
        if (used < PAGE_SIZE) {
            rc = assert_zero_region(
                case_name,
                page,
                used,
                PAGE_SIZE - used,
                "page tail");
            if (rc != 0) {
                goto out;
            }
        }

        cursor += ph.nrec;
    }

    if (cursor != expected_n) {
        fprintf(stderr, "[%s] cursor mismatch: got=%zu expected=%zu\n",
            case_name,
            cursor,
            expected_n);
        rc = -1;
        goto out;
    }

    printf("[OK] %s\n", case_name);
    rc = 0;

out:
    if (fd >= 0) {
        close(fd);
    }
    return rc;
}

static int run_dump_layout_test(const char *name, size_t rows_n)
{
    char path[128];
    StatData *rows;
    int rc;

    rows = NULL;
    rc = 0;
    snprintf(path, sizeof(path), "/tmp/%s_dump.bin", name);

    rows = make_rows(rows_n);
    if (rows_n > 0 && rows == NULL) {
        fprintf(stderr, "[%s] make_rows failed\n", name);
        return -ENOMEM;
    }

    rc = store_dump_file(path, rows, rows_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] store_dump_file failed: %d\n", name, rc);
        free(rows);
        unlink(path);
        return rc;
    }

    rc = verify_dump_layout(name, path, rows, rows_n);
    free(rows);
    unlink(path);
    return rc;
}

typedef int (*mutate_dump_fn)(int fd, size_t rows_n);

static int mutate_bad_header_page_size(int fd, size_t rows_n)
{
    uint64_t bad_page_size;
    (void)rows_n;

    bad_page_size = PAGE_SIZE / 2u;
    return pwrite_full(fd,
        &bad_page_size,
        sizeof(bad_page_size),
        (off_t)offsetof(DumpFileHdr, page_size));
}

static int mutate_bad_page_no(int fd, size_t rows_n)
{
    uint32_t bad_page_no;
    (void)rows_n;

    bad_page_no = 1u;
    return pwrite_full(fd,
        &bad_page_no,
        sizeof(bad_page_no),
        (off_t)PAGE_SIZE + (off_t)offsetof(DumpPageHdr, page_no));
}

static int mutate_bad_nrec(int fd, size_t rows_n)
{
    DumpFileHdr hdr;
    uint32_t bad_nrec;

    memset(&hdr, 0, sizeof(hdr));
    init_dump_file_hdr(&hdr, (uint64_t)rows_n);
    bad_nrec = hdr.rec_per_page + 1u;

    return pwrite_full(fd,
        &bad_nrec,
        sizeof(bad_nrec),
        (off_t)PAGE_SIZE + (off_t)offsetof(DumpPageHdr, nrec));
}

static int mutate_truncate_last_byte(int fd, size_t rows_n)
{
    struct stat st;
    (void)rows_n;

    memset(&st, 0, sizeof(st));
    if (fstat(fd, &st) != 0) {
        return -errno;
    }
    if (st.st_size <= 0) {
        return -EINVAL;
    }
    if (ftruncate(fd, st.st_size - 1) != 0) {
        return -errno;
    }
    return 0;
}

static int run_corruption_test(
    const char *name,
    size_t rows_n,
    mutate_dump_fn mutate)
{
    char path[128];
    StatData *rows;
    StatData *got;
    size_t got_n;
    int fd;
    int rc;

    rows = NULL;
    got = NULL;
    got_n = 0;
    fd = -1;
    rc = 0;

    snprintf(path, sizeof(path), "/tmp/%s_dump.bin", name);

    rows = make_rows(rows_n);
    if (rows_n > 0 && rows == NULL) {
        fprintf(stderr, "[%s] make_rows failed\n", name);
        return -ENOMEM;
    }

    rc = store_dump_file(path, rows, rows_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] store_dump_file failed: %d\n", name, rc);
        free(rows);
        unlink(path);
        return rc;
    }

    fd = open(path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "[%s] open(%s) failed: %d\n", name, path, errno);
        free(rows);
        unlink(path);
        return -errno;
    }

    rc = mutate(fd, rows_n);
    if (rc != 0) {
        fprintf(stderr, "[%s] mutate failed: %d\n", name, rc);
        close(fd);
        free(rows);
        unlink(path);
        return rc;
    }

    if (fsync(fd) != 0) {
        rc = -errno;
        fprintf(stderr, "[%s] fsync failed: %d\n", name, rc);
        close(fd);
        free(rows);
        unlink(path);
        return rc;
    }

    close(fd);
    fd = -1;

    rc = load_dump_file(path, &got, &got_n);
    if (rc == 0) {
        fprintf(stderr,
            "[%s] expected load_dump_file to fail, but it succeeded (rows=%zu)\n",
            name,
            got_n);
        free(got);
        free(rows);
        unlink(path);
        return -1;
    }

    printf("[OK] %s\n", name);

    free(rows);
    unlink(path);
    return 0;
}

int run_dump_tests(void)
{
    DumpFileHdr layout;
    size_t rec_per_page;
    size_t total;
    size_t passed;
    int rc;

    memset(&layout, 0, sizeof(layout));
    init_dump_file_hdr(&layout, 0);
    rec_per_page = layout.rec_per_page;

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

    total += 1;
    rc = run_dump_layout_test("dump_layout_empty", 0);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_layout_test("dump_layout_single", 1);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_layout_test("dump_layout_last_partial", rec_per_page - 1u);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_layout_test("dump_layout_exact_page", rec_per_page);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_layout_test("dump_layout_page_plus_one", rec_per_page + 1u);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_layout_test(
        "dump_layout_morsel_boundary",
        (size_t)MORSEL_SIZE * rec_per_page);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_dump_layout_test(
        "dump_layout_morsel_boundary_plus_one",
        (size_t)MORSEL_SIZE * rec_per_page + 1u);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_corruption_test(
        "dump_corrupt_header_page_size",
        1,
        mutate_bad_header_page_size);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_corruption_test(
        "dump_corrupt_page_no",
        rec_per_page + 1u,
        mutate_bad_page_no);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_corruption_test(
        "dump_corrupt_nrec",
        rec_per_page + 1u,
        mutate_bad_nrec);
    if (rc == 0) {
        passed += 1;
    }

    total += 1;
    rc = run_corruption_test(
        "dump_truncated_file",
        rec_per_page + 1u,
        mutate_truncate_last_byte);
    if (rc == 0) {
        passed += 1;
    }

    printf("dump tests: %zu/%zu passed\n", passed, total);
    if (passed != total) {
        return 1;
    }
    return 0;
}