#include "print_dump.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void mode_to_binary(unsigned int mode, char *buf, size_t bufsz) {
    assert(buf != NULL);
    assert(bufsz > 0);

    if (bufsz < 2) {
        if (bufsz == 1) {
            buf[0] = '\0';
        }
        return;
    }

    if (mode == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    unsigned int msb = 0;
    for (unsigned int i = 0; i < 32; ++i) {
        if ((mode >> i) != 0) {
            msb = i;
        }
    }

    size_t pos = 0;
    for (int i = (int)msb; i >= 0; --i) {
        if (pos + 1 >= bufsz) {
            break;
        }
        buf[pos++] = ((mode >> i) & 1u) ? '1' : '0';
    }
    buf[pos] = '\0';
}

void print_dump_table(FILE *out, const StatData *rows, size_t n, size_t limit) {
    if (out == NULL) {
        return;
    }

    fprintf(out, "%-16s %-12s %-14s %-8s %-8s\n",
            "id", "count", "cost", "primary", "mode");

    if (rows == NULL || n == 0 || limit == 0) {
        return;
    }

    size_t to_print = (n < limit) ? n : limit;

    for (size_t i = 0; i < to_print; ++i) {
        char mode_buf[sizeof(unsigned int) * 8 + 1];
        mode_to_binary(rows[i].mode, mode_buf, sizeof(mode_buf));

        fprintf(out,
                "0x%lx %-12d %-14.3e %-8c %-8s\n",
                (unsigned long)rows[i].id,
                rows[i].count,
                (double)rows[i].cost,
                rows[i].primary ? 'y' : 'n',
                mode_buf);
    }
}