#include "load_planner.h"
#include <string.h>

int j2_load_plan_build(j2_load_plan_t *plan, const j2_segment_t *segs, size_t nsegs,
                       uint32_t entry_va, uint32_t virtBase, uint32_t physBase,
                       uint32_t ram_min, uint32_t ram_max) {
    if (!plan || !segs) return 0;
    memset(plan, 0, sizeof(*plan));
    plan->status = J2_LOAD_OK;
    plan->entry_va = entry_va;
    /* overlap check */
    for (size_t i = 0; i < nsegs; i++)
        for (size_t j = i+1; j < nsegs; j++) {
            uint32_t a0=segs[i].vmaddr, a1=segs[i].vmaddr+segs[i].vmsize;
            uint32_t b0=segs[j].vmaddr, b1=segs[j].vmaddr+segs[j].vmsize;
            if (a0 < b1 && b0 < a1) { plan->status = J2_LOAD_OVERLAP; return 0; }
        }
    uint32_t va_min = 0xFFFFFFFF, va_max = 0;
    for (size_t i = 0; i < nsegs && i < 8; i++) {
        j2_load_region_t *r = &plan->regions[plan->nregions++];
        r->va_start = segs[i].vmaddr;
        r->va_end = segs[i].vmaddr + segs[i].vmsize;
        r->fileoff = segs[i].fileoff;
        r->filesize = segs[i].filesize;
        r->vmsize = segs[i].vmsize;
        r->zero_fill = (segs[i].filesize < segs[i].vmsize) ? 1 : 0;
        if (!j2_va_to_pa(r->va_start, virtBase, physBase, &r->pa_start)) { plan->status = J2_LOAD_BAD_ARCH; return 0; }
        r->pa_end = r->pa_start + r->vmsize;
        if (r->pa_start < ram_min || r->pa_end > ram_max) { plan->status = J2_LOAD_OOB; return 0; }
        if (r->va_start < va_min) va_min = r->va_start;
        if (r->va_end > va_max) va_max = r->va_end;
    }
    plan->total_va_span = va_max - va_min;
    /* entry in executable region */
    int entry_ok = 0;
    for (size_t i = 0; i < plan->nregions; i++)
        if (j2_macho_entry_in_segment(entry_va, plan->regions[i].va_start, plan->regions[i].vmsize)) { entry_ok = 1; break; }
    if (!entry_ok) { plan->status = J2_LOAD_BAD_ENTRY; return 0; }
    if (!j2_va_to_pa(entry_va, virtBase, physBase, &plan->entry_pa)) { plan->status = J2_LOAD_BAD_ARCH; return 0; }
    /* topOfKernelData unknown by default */
    plan->topk_known = 0;
    plan->topk_pa = 0;
    return 1;
}

int j2_load_plan_simulate(const j2_load_plan_t *plan, uint8_t *mem, size_t memsize, const uint8_t *src) {
    return j2_load_plan_simulate_at(plan, mem, memsize, src, 0);
}

int j2_load_plan_simulate_at(const j2_load_plan_t *plan, uint8_t *mem, size_t memsize, const uint8_t *src, uint32_t base_pa) {
    if (!plan || !mem) return 0;
    for (size_t i = 0; i < plan->nregions; i++) {
        const j2_load_region_t *r = &plan->regions[i];
        uint32_t pa = r->pa_start;
        if (base_pa != 0) pa = base_pa + (r->va_start - 0x80000000); /* explicit base override not used; keep pa_start */
        (void)pa;
        size_t off = r->pa_start;
        if (off + r->vmsize > memsize) return 0;
        if (r->filesize > 0 && src) memcpy(mem + off, src + r->fileoff, r->filesize);
        if (r->zero_fill) memset(mem + off + r->filesize, 0, r->vmsize - r->filesize);
    }
    return 1;
}

int j2_topk_compute(const j2_load_plan_t *plan, uint32_t virtBase, uint32_t physBase, uint32_t *out_pa, int *known) {
    if (!plan || !out_pa || !known) return 0;
    *known = 0;
    *out_pa = 0;
    if (plan->nregions == 0) return 0;
    uint32_t max_va_end = 0;
    for (size_t i = 0; i < plan->nregions; i++)
        if (plan->regions[i].va_end > max_va_end) max_va_end = plan->regions[i].va_end;
    if (max_va_end <= virtBase) return 0;
    uint32_t off = max_va_end - virtBase;
    if (physBase + off < physBase) return 0;
    /* candidate only: translation tables may extend beyond; mark unknown */
    *out_pa = physBase + off;
    *known = 0; /* not confirmed: BSS/tablas may extend */
    return 1;
}