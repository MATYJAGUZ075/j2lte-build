#ifndef J2_LOAD_PLANNER_H
#define J2_LOAD_PLANNER_H
#include <stdint.h>
#include <stddef.h>
#include "macho/full/macho_full.h"

typedef enum { J2_LOAD_OK=0, J2_LOAD_BAD_ARCH, J2_LOAD_BAD_CMD, J2_LOAD_OVERLAP, J2_LOAD_OOB, J2_LOAD_BAD_ENTRY, J2_LOAD_UNKNOWN_TOPK } j2_load_status_t;

typedef struct {
    uint32_t va_start, va_end;
    uint32_t pa_start, pa_end;
    uint32_t fileoff, filesize;
    uint32_t vmsize;
    int zero_fill;
} j2_load_region_t;

typedef struct {
    j2_load_region_t regions[8];
    size_t nregions;
    uint32_t entry_va;
    uint32_t entry_pa;
    uint32_t total_va_span;
    uint32_t total_pa_span;
    uint32_t topk_pa;
    int topk_known;
    j2_load_status_t status;
} j2_load_plan_t;

int j2_load_plan_build(j2_load_plan_t *plan, const j2_segment_t *segs, size_t nsegs,
                       uint32_t entry_va, uint32_t virtBase, uint32_t physBase,
                       uint32_t ram_min, uint32_t ram_max);
int j2_load_plan_simulate(const j2_load_plan_t *plan, uint8_t *mem, size_t memsize, const uint8_t *src);
int j2_load_plan_simulate_at(const j2_load_plan_t *plan, uint8_t *mem, size_t memsize, const uint8_t *src, uint32_t base_pa);
int j2_topk_compute(const j2_load_plan_t *plan, uint32_t virtBase, uint32_t physBase, uint32_t *out_pa, int *known);

#endif