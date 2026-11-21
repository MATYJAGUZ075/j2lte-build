#include <stdio.h>
#include "macho_full.h"
int main(void) {
    unsigned char buf[1024] = {0xCE,0xFA,0xED,0xFE, 12,0,0,0, 12,0,0,0, 2,0,0,0, 1,0,0,0};
    j2_segment_t segs[4]; size_t n=0;
    printf("parsed=%d count=%zu\n", j2_parse_segments(buf, sizeof(buf), segs, 4, &n), n);
    return 0;
}
