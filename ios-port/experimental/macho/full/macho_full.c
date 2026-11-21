#include "macho_full.h"
#include <string.h>
int j2_parse_segments(const void *data, size_t size, j2_segment_t *out, size_t max, size_t *count) {
    if (!data || !out || !count || size < 28) return 0;
    const uint8_t *b = data;
    uint32_t ncmds = *(uint32_t*)(b+16);
    size_t off = 28;
    size_t n = 0;
    for (uint32_t i=0;i<ncmds && off + 56 <= size;i++) {
        uint32_t cmd = *(uint32_t*)(b+off);
        uint32_t cmdsize = *(uint32_t*)(b+off+4);
        if (cmd == 1 && n < max) {
            memcpy(out[n].name, b+off+8, 16);
            out[n].name[15]=0;
            out[n].vmaddr = *(uint32_t*)(b+off+24);
            out[n].vmsize = *(uint32_t*)(b+off+28);
            out[n].fileoff = *(uint32_t*)(b+off+32);
            out[n].filesize = *(uint32_t*)(b+off+36);
            n++;
        }
        off += cmdsize;
    }
    *count = n;
    return 1;
}
