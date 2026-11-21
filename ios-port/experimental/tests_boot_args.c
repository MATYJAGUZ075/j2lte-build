#include <stdio.h>
#include <stddef.h>
#include "boot_args_builder.h"

int main(void) {
    printf("sizeof j2_boot_args_t = %zu\n", sizeof(j2_boot_args_t));
    printf("offset deviceTreeP = %zu\n", offsetof(j2_boot_args_t, deviceTreeP));
    return 0;
}
