#ifndef J2_MEMPLAN_H
#define J2_MEMPLAN_H
#include <stdint.h>
typedef struct {
    uint32_t physBase;
    uint32_t virtBase;
    uint32_t memSize;
    uint32_t kernel_code_start, kernel_code_end;
    uint32_t kernel_data_start, kernel_data_end;
    uint32_t dt_size, ramdisk_size, boot_args_size;
    uint32_t topOfKernelData;
    int topk_known;
} j2_memplan_t;

int j2_memplan_validate(const j2_memplan_t *p);
int j2_memplan_compute_topk(j2_memplan_t *p);
#endif
