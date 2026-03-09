#pragma once

#include <stddef.h>
#include "statdata.h"

int load_dump_fd(int fd, StatData **rows, size_t *n);
int load_dump_file(const char *path, StatData **rows, size_t *n);