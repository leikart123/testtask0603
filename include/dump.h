#pragma once
#include <stdint.h>
#define PAGE_SIZE 8192
#define MORSEL_SIZE 128
typedef struct {
        uint64_t page_size;
        uint32_t sizeof_statdata;
        uint32_t alignof_statdata;
        uint64_t total_records;
        uint64_t data_pages;
        uint32_t data_off;
        uint32_t rec_per_page;
    } DumpFileHdr;

typedef struct {
        uint32_t nrec;
        uint32_t page_no;
    } DumpPageHdr;

int validate_dump_hdr(const DumpFileHdr *h);
void init_dump_file_hdr(DumpFileHdr *h, uint64_t total_records);