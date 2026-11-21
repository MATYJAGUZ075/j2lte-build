#ifndef J2_MCT_H
#define J2_MCT_H
#include <stdint.h>
/* Verified via /proc/iomem and device-tree on SM-J200M */
#define J2_MCT_BASE 0x100B0000U
#define J2_MCT_SIZE 0x1000U
int j2_mct_base_valid(uint32_t base);
#endif
