#ifndef J2_MCT_H
#define J2_MCT_H
#include <stdint.h>
/* Verified via /proc/iomem and device-tree on SM-J200M */
#define J2_MCT_BASE 0x100B0000U
#define J2_MCT_SIZE 0x1000U
/* Global counter registers (Exynos MCT) */
#define J2_MCT_G_CNT_L 0x100U
#define J2_MCT_G_CNT_U 0x104U

int j2_mct_base_valid(uint32_t base);
uint64_t j2_mct_read_counter(volatile uint32_t *base);
#endif
