#include "macho_loader.h"
int j2_macho_validate_header(const void *data, size_t size) {
    if (!data || size < 28) return 0;
    const uint32_t *w = data;
    if (w[0] != 0xFEEDFACEU) return 0;
    if (w[1] != 12) return 0;
    if (w[2] != 12) return 0;
    if (w[3] != 2) return 0;
    return 1;
}
int j2_macho_entry_in_segment(uint32_t entry, uint32_t vmaddr, uint32_t vmsize) {
    if (vmsize == 0) return 0;
    if (entry < vmaddr) return 0;
    if (entry >= vmaddr + vmsize) return 0;
    if (vmaddr + vmsize < vmaddr) return 0;
    return 1;
}
