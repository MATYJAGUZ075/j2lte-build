/*
 * Samsung Exynos 3475 (J2) platform definitions for XNU.
 *
 * Based on MT6580.h pattern, adapted for Exynos 3475 (SM-J200M).
 * Cortex-A7 (ARMv7-A), GIC-400, MCT timer, PMU SMP, eMMC (dwmmc).
 * No ARM generic timer - uses Samsung MCT.
 */

#ifndef _PEXPERT_ARM_J2_H
#define _PEXPERT_ARM_J2_H

/* SoC physical aperture.  The stock Exynos 3475 device tree has no Apple
 * "arm-io" node, so the platform expert uses this base for the few
 * devices it needs during early bring-up. */
#define J2_SOC_PHYS_BASE		0x10000000

/* GIC-400 (compatible "arm,cortex-a15-gic") */
#define J2_GIC_DIST_BASE		0x10C01000
#define J2_GIC_CPU_BASE		0x10C02000
#define J2_GIC_DIST_SIZE		0x1000
#define J2_GIC_CPU_SIZE		0x2000
#define J2_GIC_CPU_OFFSET		0x1000

/* GIC-400 register offsets (relative to the distributor / CPU interface) */
#define J2_GICD_CTLR		0x000
#define J2_GICD_IGROUPR0		0x080
#define J2_GICD_ISENABLER0		0x100
#define J2_GICD_IPRIORITYR		0x400
#define J2_GICD_ICFGR1		0xC04
#define J2_GICC_CTLR		0x000
#define J2_GICC_PMR			0x004
#define J2_GICC_BPR			0x008
#define J2_GICC_IAR			0x00C
#define J2_GICC_EOIR		0x010
#define J2_GICC_DIR			0x1000

/* Samsung MCT (Multi-Core Timer) - NO ARM generic timer on Exynos 3475 */
#define J2_MCT_BASE			0x100B0000
#define J2_MCT_SIZE			0x800
/* MCT interrupt map: 11 channels -> GIC PPI 102-113 (0-10 + PPI base) */
#define J2_MCT_IRQ_BASE		102
#define J2_MCT_NUM_CHANNELS	11
/* MCT clock frequency - Exynos 3475 uses 26 MHz fin_pll (like S8) */
#define J2_MCT_FIN_PLL		26000000
#define J2_CNTFRQ			26000000

/* Early platform clock estimates.  Refined once clock driver available. */
#define J2_CPU_FREQ			1300000000
#define J2_BUS_FREQ			400000000
#define J2_PRF_FREQ			100000000

/* Stock panel (s6e88a0 540x960 Super AMOLED via DECON) */
#define J2_PANEL_WIDTH		540
#define J2_PANEL_HEIGHT		960
#define J2_PANEL_ROWBYTES		(540 * 4)
#define J2_PANEL_DEPTH		32
/* Framebuffer address left by S-Boot - will be read from DTB */
#define J2_FB_BASE			0x9C000000	/* typical S-Boot FB, overridden by DTB */

/* Samsung watchdog (S3C2410 compatible) */
#define J2_WDT_BASE			0x101C0000
#define J2_WDT_IRQ			100

/* UARTs (Samsung S3C2410 compatible) */
#define J2_UART0_BASE		0x13800000
#define J2_UART1_BASE		0x13810000
#define J2_UART2_BASE		0x13820000

/* Display / DECON */
#define J2_DISP_BASE		0x14830000
#define J2_DECON_SIZE		0x10000

/* eMMC / dwmmc (Synopsys DesignWare MMC) x3 controllers */
#define J2_DWMMC0_BASE		0x13540000
#define J2_DWMMC1_BASE		0x13550000
#define J2_DWMMC2_BASE		0x13560000

/* I2C controllers (HSI2C + standard I2C) */
#define J2_HSI2C0_BASE		0x13870000
#define J2_HSI2C1_BASE		0x13880000
#define J2_I2C0_BASE		0x13830000
#define J2_I2C1_BASE		0x13840000
#define J2_I2C2_BASE		0x13850000
#define J2_I2C3_BASE		0x13860000
#define J2_I2C4_BASE		0x138A0000
#define J2_I2C5_BASE		0x138B0000
#define J2_I2C6_BASE		0x13940000
#define J2_I2C7_BASE		0x13950000

/* PMIC SM5703 on HSI2C0 @ 0x49 */
#define J2_PMIC_I2C_BASE		J2_HSI2C0_BASE
#define J2_PMIC_I2C_ADDR		0x49

/* RTC (S3C2410 compatible) */
#define J2_RTC_BASE			0x101E0000
#define J2_RTC_IRQ			?

/* Clock controller */
#define J2_CLK_BASE			0x10010000

/* Power Management Unit (PMU) for SMP */
#define J2_PMU_BASE			0x10004000	/* EXYNOS_PMU_BASE from Linux */

/* ARM generic timer NOT present on Exynos 3475 - uses MCT instead */
/* #define J2_CNTFRQ			19200000  -- NOT USED */
/* #define J2_TIMER_IRQ		27        -- NOT USED (virtual timer PPI) */

/* Early platform clock estimates */
#define J2_CPU_FREQ			1300000000
#define J2_BUS_FREQ			400000000
#define J2_PRF_FREQ			100000000

/* Panel (s6e88a0 540x960 Super AMOLED) */
#define J2_PANEL_WIDTH		540
#define J2_PANEL_HEIGHT		960
#define J2_PANEL_ROWBYTES		(540 * 4)
#define J2_PANEL_DEPTH		32

#ifndef ASSEMBLER

#include <stdint.h>

static inline uint32_t
j2_read32(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static inline void
j2_write32(uintptr_t addr, uint32_t value)
{
	*(volatile uint32_t *)addr = value;
}

#endif /* !ASSEMBLER */

#endif /* ! _PEXPERT_ARM_J2_H */