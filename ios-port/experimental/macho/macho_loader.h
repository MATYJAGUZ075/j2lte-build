#ifndef J2_MACHO_LOADER_H
#define J2_MACHO_LOADER_H
#include <stdint.h>
#include <stddef.h>
int j2_macho_validate_header(const void *data, size_t size);
#endif
