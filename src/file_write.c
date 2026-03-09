#include "buffer_pool.h"
#include "dump.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <stddef.h>

int write_single_page(int fd, const void *page, size_t page_size) {
    assert(fd >= 0);
    assert(page != NULL);
    assert(page_size > 0);

    const uint8_t *p = (const uint8_t *)page;
    size_t done = 0;

    while (done < page_size) {
        ssize_t n = pwrite(fd, p + done, page_size - done, (off_t)done);
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

int write_morsel(int fd, const Buffer *buf, off_t offset) {
    assert(fd >= 0);
    assert(buf != NULL);
    assert(buf->data != NULL);
    assert(buf->used <= buf->capacity);

    if (buf->used == 0) {
        return 0;
    }

    const uint8_t *p = (const uint8_t *)buf->data;
    size_t done = 0;

    while (done < buf->used) {
        ssize_t n = pwrite(fd, p + done, buf->used - done, offset + (off_t)done);
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