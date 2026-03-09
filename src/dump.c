#include "dump.h"
#include "statdata.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "log.h"
#include "errno.h"

static inline uint32_t align_up_u32(uint32_t x, uint32_t a) {
    assert(a != 0);
    assert((a & (a - 1)) == 0);
    return (x + a - 1u) & ~(a - 1u);
}

int validate_dump_hdr(const DumpFileHdr *h) {
    if (!h) 
        return -EINVAL;

    if (h->page_size != PAGE_SIZE) 
        return -EINVAL;
    if (h->sizeof_statdata != sizeof(StatData)) 
        return -EINVAL;
    if (h->alignof_statdata != _Alignof(StatData)) 
        return -EINVAL;
    if (h->data_off < sizeof(DumpPageHdr)) 
        return -EINVAL;
    if (h->data_off >= PAGE_SIZE) 
        return -EINVAL;
    if ((h->data_off % _Alignof(StatData)) != 0) 
        return -EINVAL;
    if (h->rec_per_page == 0) 
        return -EINVAL;
    {
        uint32_t max_rec_per_page =
            (uint32_t)((PAGE_SIZE - h->data_off) / sizeof(StatData));
        if (h->rec_per_page > max_rec_per_page) 
            return -EINVAL;
    }
    return 0;
}

void init_dump_file_hdr(DumpFileHdr *h, uint64_t total_records) {
    memset(h, 0, sizeof(*h));

    h->page_size        = PAGE_SIZE;
    h->alignof_statdata = (uint32_t)_Alignof(StatData);
    h->sizeof_statdata  = (uint32_t)sizeof(StatData);
    h->total_records    = total_records;
    h->data_off = align_up_u32((uint32_t)sizeof(DumpPageHdr),
                              (uint32_t)_Alignof(StatData));
    h->rec_per_page = (h->page_size - h->data_off) / (uint32_t)sizeof(StatData);

    assert(h->data_off < PAGE_SIZE);
    assert((PAGE_SIZE - h->data_off) >= sizeof(StatData));
    assert(h->rec_per_page > 0);

#ifdef DEBUG
    log_dump_file_hdr(h);
#endif
}
