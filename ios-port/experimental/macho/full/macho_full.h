#ifndef J2_MACHO_FULL_H
#define J2_MACHO_FULL_H
#include <stdint.h>
#include <stddef.h>
typedef struct { uint32_t vmaddr, vmsize, fileoff, filesize; char name[16]; } j2_segment_t;
int j2_parse_segments(const void *data, size_t size, j2_segment_t *out, size_t max, size_t *count);
#endif
