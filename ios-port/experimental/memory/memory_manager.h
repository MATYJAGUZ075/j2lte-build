#ifndef J2_MEMORY_MANAGER_H
#define J2_MEMORY_MANAGER_H
#include <stdint.h>
#include <stddef.h>

#define J2_MEM_REGIONS 4

typedef enum { J2_MEM_UNKNOWN=0, J2_MEM_RAM, J2_MEM_RESERVED, J2_MEM_OCCUPIED } j2_mem_kind_t;

typedef struct {
    uint32_t start, size;
    j2_mem_kind_t kind;
    char name[32];
} j2_mem_region_t;

typedef struct {
    j2_mem_region_t regions[J2_MEM_REGIONS];
    size_t nregions;
} j2_mem_map_t;

int j2_memory_validate_region(uint32_t start, uint32_t size, uint32_t min, uint32_t max);
int j2_mem_map_init(j2_mem_map_t *m);
int j2_mem_map_add(j2_mem_map_t *m, uint32_t start, uint32_t size, j2_mem_kind_t kind, const char *name);
int j2_mem_ranges_overlap(uint32_t a0, uint32_t a1, uint32_t b0, uint32_t b1);
int j2_mem_is_within_ram(const j2_mem_map_t *m, uint32_t start, uint32_t size);
int j2_mem_reserve(j2_mem_map_t *m, uint32_t start, uint32_t size, const char *name);

#endif