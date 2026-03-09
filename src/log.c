#include "statdata.h"
#include "dump.h"
#include <stdio.h>

static void mode_to_bin(unsigned mode, char out[16]) {
    if (mode == 0) { out[0] = '0'; out[1] = '\0'; return; }

    int i = 0;
    unsigned int m = mode;
    while (m && i < 15) {
        out[i++] = (m & 1u) ? '1' : '0';
        m >>= 1u;
    }
    for (int l = 0, r = i - 1; l < r; ++l, --r) {
        char tmp = out[l]; out[l] = out[r]; out[r] = tmp;
    }
    out[i] = '\0';
}

void log_dump_file_hdr(DumpFileHdr *h) {
    printf("[DEBUG] DumpFileHdr: page size = %u \n \
        sizeof StatData = %u \n \
        alignof StatData = %u \n \
        pagehdr_sz = %zu \n \
        data offset = %u \n \
        page capacity in bytes= %u \n \
        recs per page = %u\n \
        total records = %lu\n",
       (unsigned)h->page_size,
       (unsigned)h->sizeof_statdata,
       (unsigned)h->alignof_statdata,
       sizeof(DumpPageHdr),
       (unsigned)h->data_off,
       (unsigned)(h->page_size - h->data_off),
       (unsigned)h->rec_per_page,
       h->total_records);
}

void log_statdata(const StatData *s)
{
    char mode_bin[16];
    mode_to_bin((unsigned)s->mode, mode_bin);

    printf("[DEBUG] StatData:\n \
        id = %ld \n \
        count = %d\n \
        cost = %e\n \
        mode = %s \n",
        (long)s->id,
        (int)s->count,
        (double)s->cost,
        (char *)mode_bin);
}