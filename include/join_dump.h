#pragma once

#include <stddef.h>
#include "statdata.h"

int join_dump( const StatData *a, size_t na, const StatData *b, size_t nb, StatData **out, size_t *nout);