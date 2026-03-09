#pragma once

#include <stddef.h>
#include <stdio.h>
#include "statdata.h"

void print_dump_table(FILE *out, const StatData *rows, size_t n, size_t limit);