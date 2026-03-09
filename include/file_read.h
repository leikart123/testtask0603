#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "buffer_pool.h"

int read_single_page(int fd, void *page, size_t page_size);
int read_morsel(int fd, Buffer *buf, uint32_t offset);
