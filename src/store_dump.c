#include "store_dump.h"

#include "buffer_pool.h"
#include "dump.h"
#include "file_write.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint64_t ceil_div_u64(uint64_t n, uint64_t d) {
    return d ? (n + d - 1u) / d : 0;
}

static void fill_header_page(void *page, const DumpFileHdr *hdr) {
    assert(page != NULL);
    assert(hdr != NULL);

    memset(page, 0, PAGE_SIZE);
    memcpy(page, hdr, sizeof(*hdr));
}

static size_t fill_data_page(
    void *page,
    const DumpFileHdr *hdr,
    uint32_t page_no,
    const StatData *rows,
    size_t total_rows,
    size_t cursor)
{
    assert(page != NULL);
    assert(hdr != NULL);
    assert(rows != NULL || total_rows == 0);

    memset(page, 0, PAGE_SIZE);

    DumpPageHdr ph;
    memset(&ph, 0, sizeof(ph));
    ph.page_no = page_no;

    size_t left = total_rows - cursor;
    size_t take = left < (size_t)hdr->rec_per_page ? left : (size_t)hdr->rec_per_page;
    ph.nrec = (uint32_t)take;

    memcpy(page, &ph, sizeof(ph));

    if (take > 0) {
        memcpy((uint8_t *)page + hdr->data_off,
               rows + cursor,
               take * sizeof(StatData));
    }

    return take;
}

int store_dump_fd(int fd, const StatData *rows, size_t n) {
    if (fd < 0) {
        return -EINVAL;
    }
    if (n > 0 && rows == NULL) {
        return -EINVAL;
    }

    int rc = 0;
    DumpFileHdr hdr;
    init_dump_file_hdr(&hdr, (uint64_t)n);
    hdr.data_pages = ceil_div_u64((uint64_t)n, (uint64_t)hdr.rec_per_page);

    /* для простоты буферпул на 1 morsel, позже можно добавть многопоточную запись */
    const uint64_t total_pages = 1u + hdr.data_pages;
    BufferPool *pool = new_buffer_pool(1, MORSEL_SIZE * PAGE_SIZE, PAGE_SIZE);
    if (!pool) {
        return -ENOMEM;
    }

    size_t row_cursor = 0;
    uint64_t global_page_idx = 0;

    while (global_page_idx < total_pages) {
        Buffer buf;
        rc = bufferpool_try_acquire(pool, &buf);
        if (rc != 0) {
            if (rc == -EAGAIN) {
                continue;
            }
            break;
        }

        memset(buf.data, 0, buf.capacity);
        buf.used = 0;

        size_t local_pages = 0;
        const uint64_t morsel_first_page = global_page_idx;

        while (local_pages < MORSEL_SIZE && global_page_idx < total_pages) {
            void *page = (uint8_t *)buf.data + local_pages * PAGE_SIZE;

            if (global_page_idx == 0) {
                fill_header_page(page, &hdr);
            } else {
                uint32_t data_page_no = (uint32_t)(global_page_idx - 1u);
                size_t written = fill_data_page(
                    page,
                    &hdr,
                    data_page_no,
                    rows,
                    n,
                    row_cursor
                );
                row_cursor += written;
            }

            ++local_pages;
            ++global_page_idx;
        }

        buf.used = local_pages * PAGE_SIZE;

        rc = write_morsel(fd, &buf, (off_t)(morsel_first_page * PAGE_SIZE));

        bufferpool_release(pool, &buf);

        if (rc != 0) {
            break;
        }
    }

    if (rc == 0 && fsync(fd) != 0) {
        rc = -errno;
    }

    free_buffer_pool(pool);
    return rc;
}

int store_dump_file(const char *path, const StatData *rows, size_t n) {
    if (!path) {
        return -EINVAL;
    }

    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) {
        return -errno;
    }

    int rc = store_dump_fd(fd, rows, n);

    if (close(fd) != 0 && rc == 0) {
        rc = -errno;
    }

    return rc;
}