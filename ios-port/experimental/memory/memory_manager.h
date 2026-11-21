#ifndef J2_MEMORY_MANAGER_H
#define J2_MEMORY_MANAGER_H
#include <stdint.h>
int j2_memory_validate_region(uint32_t start, uint32_t size, uint32_t min, uint32_t max);
#endif
