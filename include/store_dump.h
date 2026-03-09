#pragma once

#include <stddef.h>
#include "statdata.h"

int store_dump_fd(int fd, const StatData *rows, size_t n);
int store_dump_file(const char *path, const StatData *rows, size_t n);