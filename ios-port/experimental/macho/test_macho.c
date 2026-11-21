#include <stdio.h>
#include "macho_loader.h"
int main(void) {
    unsigned char h[28] = {0xCE,0xFA,0xED,0xFE, 12,0,0,0, 12,0,0,0};
    printf("valid=%d\n", j2_macho_validate_header(h, sizeof(h)));
    return 0;
}
