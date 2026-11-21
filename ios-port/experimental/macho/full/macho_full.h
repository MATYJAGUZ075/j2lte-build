#ifndef J2_MACHO_FULL_H
#define J2_MACHO_FULL_H
#include <stdint.h>
#include <stddef.h>

#define J2_MACHO_MAGIC 0xFEEDFACEU

typedef struct {
    uint32_t vmaddr, vmsize, fileoff, filesize;
    uint32_t maxprot, initprot;
    char name[16];
    uint32_t nsects;
} j2_segment_t;

typedef struct {
    char sectname[16];
    char segname[16];
    uint32_t addr, size, offset, align;
    uint32_t flags;
} j2_section_t;

int j2_parse_macho_header(const void *data, size_t size, uint32_t *ncmds, uint32_t *sizeofcmds);
int j2_parse_segments(const void *data, size_t size, j2_segment_t *out, size_t max, size_t *count);
int j2_segments_overlap(const j2_segment_t *a, size_t na);
int j2_va_to_pa(uint32_t va, uint32_t virtBase, uint32_t physBase, uint32_t *pa);

#endif