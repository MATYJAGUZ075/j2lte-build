#ifndef J2_GIC_H
#define J2_GIC_H
#include <stdint.h>
/* Verified via /proc/iomem and device-tree on SM-J200M */
#define J2_GIC_BASE 0x10C01000U
#define J2_GIC_SIZE 0x10000U
/* CPU interface commonly at distributor+0x2000 on Exynos */
#define J2_GICC_BASE (J2_GIC_BASE + 0x2000U)

/* GICv2 distributor registers (offsets) */
#define J2_GICD_CTLR 0x000U
#define J2_GICD_TYPER 0x004U
#define J2_GICD_ISENABLER 0x100U
#define J2_GICD_ICENABLER 0x180U
#define J2_GICD_IPRIORITYR 0x400U
/* GICv2 CPU interface registers (offsets) */
#define J2_GICC_CTLR 0x000U
#define J2_GICC_PMR 0x004U
#define J2_GICC_IAR 0x00CU
#define J2_GICC_EOIR 0x010U

#define J2_GICD_CTLR_ENABLE 1U

int j2_gic_base_valid(uint32_t base);
void j2_gic_init(volatile uint32_t *dist, volatile uint32_t *cpu);
void j2_gic_enable_irq(volatile uint32_t *dist, uint32_t irq);
void j2_gic_disable_irq(volatile uint32_t *dist, uint32_t irq);
uint32_t j2_gic_ack(volatile uint32_t *cpu);
void j2_gic_set_priority(volatile uint32_t *dist, uint32_t irq, uint8_t prio);
uint32_t j2_gic_get_priority(volatile uint32_t *dist, uint32_t irq);
#endif
