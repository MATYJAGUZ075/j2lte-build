#include "macho_loader.h"
int j2_macho_validate_header(const void *data, size_t size) {
    if (!data || size < 28) return 0;
    const uint32_t *w = data;
    if (w[0] != 0xFEEDFACEU) return 0;
    if (w[1] != 12) return 0; /* ARM */
    if (w[2] != 12) return 0; /* armv7k */
    return 1;
}
