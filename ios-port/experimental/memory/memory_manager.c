#include "memory_manager.h"
int j2_memory_validate_region(uint32_t start, uint32_t size, uint32_t min, uint32_t max) {
    if (size == 0) return 0;
    if (start < min) return 0;
    if (start + size > max) return 0;
    if (start + size < start) return 0;
    return 1;
}
