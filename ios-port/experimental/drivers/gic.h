#ifndef J2_GIC_H
#define J2_GIC_H
#include <stdint.h>
/* Verified via /proc/iomem and device-tree on SM-J200M */
#define J2_GIC_BASE 0x10C00000U
#define J2_GIC_SIZE 0x10000U
int j2_gic_base_valid(uint32_t base);
#endif
