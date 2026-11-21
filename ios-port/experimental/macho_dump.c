#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "macho/macho_loader.h"

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <macho>\n", argv[0]); return 1; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = (unsigned char*)malloc(sz);
    if (!buf) { fclose(f); return 1; }
    if (fread(buf, 1, sz, f) != (size_t)sz) { free(buf); fclose(f); return 1; }
    fclose(f);
    printf("filesize=%ld\n", sz);
    j2_segment_t segs[32]; size_t n = 0;
    if (!j2_parse_segments(buf, sz, segs, 32, &n)) { fprintf(stderr, "parse fail\n"); free(buf); return 1; }
    printf("segments=%zu\n", n);
    for (size_t i = 0; i < n; i++) {
        printf("SEG %zu vmaddr=0x%08x vmsize=0x%08x fileoff=0x%08x filesize=0x%08x\n", i, segs[i].vmaddr, segs[i].vmsize, segs[i].fileoff, segs[i].filesize);
    }
    free(buf);
    return 0;
}
