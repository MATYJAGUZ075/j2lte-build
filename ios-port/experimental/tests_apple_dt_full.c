#include <stdio.h>
#include <stddef.h>
#include "apple_dt_serializer.h"

int main(void) {
    printf("align4(1)=%zu\n", j2_dt_align4(1));
    printf("align4(4)=%zu\n", j2_dt_align4(4));
    printf("node_size=%zu\n", j2_dt_node_size(NULL, 32, 0));
    return 0;
}
