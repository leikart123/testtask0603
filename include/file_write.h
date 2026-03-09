#pragma once
#include <unistd.h>
#include <stdint.h>

int write_single_page(int fd, void *page, size_t page_size);
int write_morsel(int fd, Buffer *buf, uint32_t offset);