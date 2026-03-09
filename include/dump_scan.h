#pragma once
#include "statdata.h"
#include "buffer_pool.h"

typedef int (*record_cb_t)(const StatData *row, void *arg);

int scan_dump(int fd, BufferPool *pool, record_cb_t cb, void *arg);