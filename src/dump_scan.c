#include "dump_scan.h"

#include "dump.h"
#include "file_read.h"
#include "statdata.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint64_t ceil_div_u64(uint64_t n, uint64_t d) {
    if (d == 0) {
        return 0;
    }

    return (n + d - 1) / d;
}

static uint64_t min_u64(uint64_t a, uint64_t b) {
    if (a < b) {
        return a;
    }

    return b;
}

static int alloc_aligned_page(void **out) {
    void *p;
    int rc;

    if (out == NULL) {
        return -EINVAL;
    }

    p = NULL;
    rc = posix_memalign(&p, PAGE_SIZE, PAGE_SIZE);
    if (rc != 0) {
        return -rc;
    }

    memset(p, 0, PAGE_SIZE);
    *out = p;
    return 0;
}

static int scan_dump_body(
    int fd,
    BufferPool *pool,
    const DumpFileHdr *h,
    record_cb_t cb,
    void *arg
) {
    Buffer buf;
    uint64_t n_pages;
    uint64_t n_morsels;
    uint64_t records_seen;
    int rc;

    memset(&buf, 0, sizeof(buf));

    rc = bufferpool_try_acquire(pool, &buf);
    if (rc != 0) {
        return rc;
    }

    n_pages = ceil_div_u64(h->total_records, h->rec_per_page);
    n_morsels = ceil_div_u64(n_pages, MORSEL_SIZE);
    records_seen = 0;

    for (uint64_t morsel_no = 0; morsel_no < n_morsels; ++morsel_no) {
        uint64_t start_page;
        uint64_t pages_this;
        uint64_t read_off64;

        start_page = morsel_no * MORSEL_SIZE;
        pages_this = min_u64(MORSEL_SIZE, n_pages - start_page);

        if (pages_this == 0) {
            break;
        }

        read_off64 = PAGE_SIZE + morsel_no * ((uint64_t)MORSEL_SIZE * PAGE_SIZE);
        if (read_off64 > UINT32_MAX) {
            bufferpool_release(pool, &buf);
            return -EOVERFLOW;
        }
        
        buf.used = (size_t)(pages_this * PAGE_SIZE);
        rc = read_morsel(fd, &buf, (uint32_t)read_off64);
        if (rc != 0) {
            bufferpool_release(pool, &buf);
            return rc;
        }


        for (uint64_t page_idx = 0; page_idx < pages_this; ++page_idx) {
            uint8_t *page;
            const DumpPageHdr *ph;
            const StatData *rows;
            uint64_t expected_page_no;

            page = (uint8_t *)buf.data + page_idx * PAGE_SIZE;
            ph = (const DumpPageHdr *)page;
            rows = (const StatData *)(page + h->data_off);
            expected_page_no = start_page + page_idx;

            if (ph->page_no != (uint32_t)expected_page_no) {
                bufferpool_release(pool, &buf);
                return -EINVAL;
            }

            if (ph->nrec > h->rec_per_page) {
                bufferpool_release(pool, &buf);
                return -EINVAL;
            }

            if (records_seen + ph->nrec > h->total_records) {
                bufferpool_release(pool, &buf);
                return -EINVAL;
            }

            for (uint32_t i = 0; i < ph->nrec; ++i) {
                rc = cb(&rows[i], arg);
                if (rc != 0) {
                    bufferpool_release(pool, &buf);
                    return rc;
                }
            }

            records_seen += ph->nrec;
        }
    }

    bufferpool_release(pool, &buf);

    if (records_seen != h->total_records) {
        return -EINVAL;
    }

    return 0;
}

int scan_dump(int fd, BufferPool *pool, record_cb_t cb, void *arg) {
    void *hdr_page;
    DumpFileHdr h;
    int rc;

    if (fd < 0) {
        return -EINVAL;
    }

    if (pool == NULL) {
        return -EINVAL;
    }

    if (cb == NULL) {
        return -EINVAL;
    }

    hdr_page = NULL;
    memset(&h, 0, sizeof(h));

    rc = alloc_aligned_page(&hdr_page);
    if (rc != 0) {
        return rc;
    }

    rc = read_single_page(fd, hdr_page, PAGE_SIZE);
    if (rc != 0) {
        free(hdr_page);
        return rc;
    }

    memcpy(&h, hdr_page, sizeof(h));
    free(hdr_page);

    rc = validate_dump_hdr(&h);
    if (rc != 0) {
        return rc;
    }

    return scan_dump_body(fd, pool, &h, cb, arg);
}