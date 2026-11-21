#include "macho_full.h"
#include <string.h>

int j2_parse_macho_header(const void *data, size_t size, uint32_t *ncmds, uint32_t *sizeofcmds) {
    if (!data || size < 28 || !ncmds || !sizeofcmds) return 0;
    const uint32_t *w = data;
    if (w[0] != J2_MACHO_MAGIC) return 0;
    if (w[1] != 12) return 0; /* ARM */
    if (w[2] != 12) return 0; /* armv7k */
    if (w[3] != 2) return 0; /* MH_EXECUTE */
    *ncmds = w[4];
    *sizeofcmds = w[5];
    if (*sizeofcmds > size - 28) return 0;
    if ((size_t)*ncmds * 8 > *sizeofcmds) return 0;
    return 1;
}

int j2_parse_segments(const void *data, size_t size, j2_segment_t *out, size_t max, size_t *count) {
    if (!data || !out || !count || size < 28) return 0;
    uint32_t ncmds, sizeofcmds;
    if (!j2_parse_macho_header(data, size, &ncmds, &sizeofcmds)) return 0;
    const uint8_t *b = data;
    size_t off = 28;
    size_t end = 28 + sizeofcmds;
    if (end > size) return 0;
    size_t n = 0;
    for (uint32_t i = 0; i < ncmds && off + 8 <= end; i++) {
        if (off + 8 > size) return 0;
        uint32_t cmd = *(const uint32_t*)(b + off);
        uint32_t cmdsize = *(const uint32_t*)(b + off + 4);
        if (cmdsize < 8 || off + cmdsize > end) return 0;
        if (cmd == 1) { /* LC_SEGMENT */
            if (off + 56 > size) return 0;
            if (n < max) {
                memset(&out[n], 0, sizeof(out[n]));
                memcpy(out[n].name, b + off + 8, 16);
                out[n].name[15] = 0;
                out[n].vmaddr = *(const uint32_t*)(b + off + 24);
                out[n].vmsize = *(const uint32_t*)(b + off + 28);
                out[n].fileoff = *(const uint32_t*)(b + off + 32);
                out[n].filesize = *(const uint32_t*)(b + off + 36);
                out[n].maxprot = *(const uint32_t*)(b + off + 40);
                out[n].initprot = *(const uint32_t*)(b + off + 44);
                /* overflow check */
                if (out[n].vmaddr + out[n].vmsize < out[n].vmaddr) return 0;
                if (out[n].fileoff + out[n].filesize < out[n].fileoff) return 0;
                if ((size_t)out[n].fileoff + out[n].filesize > size) return 0;
                n++;
            }
        }
        off += cmdsize;
    }
    *count = n;
    return 1;
}

int j2_segments_overlap(const j2_segment_t *a, size_t na) {
    if (!a) return 0;
    for (size_t i = 0; i < na; i++)
        for (size_t j = i + 1; j < na; j++) {
            uint32_t a0 = a[i].vmaddr, a1 = a[i].vmaddr + a[i].vmsize;
            uint32_t b0 = a[j].vmaddr, b1 = a[j].vmaddr + a[j].vmsize;
            if (a0 < b1 && b0 < a1) return 1;
        }
    return 0;
}

int j2_va_to_pa(uint32_t va, uint32_t virtBase, uint32_t physBase, uint32_t *pa) {
    if (!pa) return 0;
    if (va < virtBase) return 0;
    uint32_t off = va - virtBase;
    if (physBase + off < physBase) return 0; /* overflow */
    *pa = physBase + off;
    return 1;
}