#include "memory_manager.h"
#include <string.h>

int j2_memory_validate_region(uint32_t start, uint32_t size, uint32_t min, uint32_t max) {
    if (size == 0) return 0;
    if (start < min) return 0;
    if (start + size > max) return 0;
    if (start + size < start) return 0;
    return 1;
}

int j2_mem_map_init(j2_mem_map_t *m) {
    if (!m) return 0;
    memset(m, 0, sizeof(*m));
    return 1;
}

int j2_mem_ranges_overlap(uint32_t a0, uint32_t a1, uint32_t b0, uint32_t b1) {
    return a0 < b1 && b0 < a1;
}

int j2_mem_map_add(j2_mem_map_t *m, uint32_t start, uint32_t size, j2_mem_kind_t kind, const char *name) {
    if (!m || m->nregions >= J2_MEM_REGIONS) return 0;
    if (size == 0) return 0;
    if (start + size < start) return 0;
    j2_mem_region_t *r = &m->regions[m->nregions++];
    r->start = start; r->size = size; r->kind = kind;
    if (name) { strncpy(r->name, name, sizeof(r->name)-1); r->name[sizeof(r->name)-1]=0; }
    return 1;
}

int j2_mem_is_within_ram(const j2_mem_map_t *m, uint32_t start, uint32_t size) {
    if (!m) return 0;
    if (start + size < start) return 0;
    uint32_t end = start + size;
    for (size_t i = 0; i < m->nregions; i++) {
        if (m->regions[i].kind != J2_MEM_RAM) continue;
        uint32_t r0 = m->regions[i].start;
        uint32_t r1 = r0 + m->regions[i].size;
        if (start >= r0 && end <= r1) return 1;
    }
    return 0;
}

int j2_mem_reserve(j2_mem_map_t *m, uint32_t start, uint32_t size, const char *name) {
    if (!m) return 0;
    if (size == 0) return 0;
    if (start + size < start) return 0;
    /* check overlap with existing regions */
    for (size_t i = 0; i < m->nregions; i++) {
        uint32_t r0 = m->regions[i].start;
        uint32_t r1 = r0 + m->regions[i].size;
        if (j2_mem_ranges_overlap(start, start + size, r0, r1)) return 0;
    }
    if (m->nregions >= J2_MEM_REGIONS) return 0;
    j2_mem_region_t *r = &m->regions[m->nregions++];
    r->start = start; r->size = size; r->kind = J2_MEM_RESERVED;
    if (name) { strncpy(r->name, name, sizeof(r->name)-1); r->name[sizeof(r->name)-1]=0; }
    return 1;
}