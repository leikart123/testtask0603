#include "test_common.h"

#include <stdio.h>

int main(void)
{
    int rc_merge;
    int rc_dump;

    rc_merge = run_merge_tests();
    rc_dump = run_dump_tests();

    if (rc_merge != 0 || rc_dump != 0) {
        return 1;
    }

    printf("all test groups passed\n");

    return 0;
}