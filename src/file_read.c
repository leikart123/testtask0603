#include "file_read.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <stddef.h>

int read_single_page(int fd, void *page, size_t page_size) {
    assert(fd >= 0);
    assert(page != NULL);
    assert(page_size > 0);

    uint8_t *p = (uint8_t *)page;
    size_t done = 0;

    while (done < page_size) {
        ssize_t n = pread(fd, p + done, page_size - done, (off_t)done);
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

int read_morsel(int fd, Buffer *buf, uint32_t offset) {
    assert(fd >= 0);
    assert(buf != NULL);
    assert(buf->data != NULL);
    assert(buf->capacity > 0);
    assert(buf->used <= buf->capacity);

    size_t target = (buf->used == 0) ? buf->capacity : buf->used;
    uint8_t *p = (uint8_t *)buf->data;
    size_t done = 0;

    while (done < target) {
        ssize_t n = pread(fd, p + done, target - done, offset + (off_t)done);
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

    buf->used = target;
    return 0;
}