#include "memplan/memplan.h"

int j2_memplan_validate(const j2_memplan_t *p) {
    if (!p) return 0;
    if (p->physBase == 0 || p->virtBase == 0 || p->memSize == 0) return 0;
    if (p->kernel_data_end <= p->kernel_data_start) return 0;
    if (p->kernel_data_end < p->physBase) return 0;
    /* known fields required for topk */
    if (p->dt_size == 0 || p->boot_args_size == 0) return 0; /* ramdisk may be 0 */
    return 1;
}

int j2_memplan_compute_topk(j2_memplan_t *p) {
    if (!p) return 0;
    if (!j2_memplan_validate(p)) { p->topk_known = 0; p->topOfKernelData = 0; return 0; }
    uint32_t off = (p->kernel_data_end - p->physBase) + p->dt_size + p->ramdisk_size + p->boot_args_size;
    /* page align up */
    off = (off + 4095) & ~4095U;
    if (p->physBase + off < p->physBase) { p->topk_known = 0; return 0; }
    p->topOfKernelData = p->physBase + off;
    /* still candidate: not confirmed without full evidence */
    p->topk_known = 0;
    return 1;
}
