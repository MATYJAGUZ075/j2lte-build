#include <stdio.h>
#include "apple_dt_serializer.h"

int main(void) {
    printf("align4(5) = %zu\n", j2_dt_align4(5));
    printf("node size = %zu\n", j2_dt_node_size(NULL, 40, 0));
    return 0;
}
