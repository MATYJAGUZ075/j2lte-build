#ifndef J2_MACHO_LOADER_H
#define J2_MACHO_LOADER_H
#include <stdint.h>
#include <stddef.h>
int j2_macho_validate_header(const void *data, size_t size);
int j2_macho_entry_in_segment(uint32_t entry, uint32_t vmaddr, uint32_t vmsize);
#endif
