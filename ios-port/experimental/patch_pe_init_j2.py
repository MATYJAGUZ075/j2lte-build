import sys

p = sys.argv[1]
s = open(p).read()

needle = "\tStartIOKit(PE_state.deviceTreeHead, PE_state.bootArgs, (void *) 0, (void *) 0);\n}\n"
assert needle in s, "needle not found in pe_init.c"
assert "j2_pe_init_platform" not in s, "patch already applied"

s = s.replace(needle, "\tStartIOKit(PE_state.deviceTreeHead, PE_state.bootArgs, (void *) 0, (void *) 0);\n\n\t/* J2 platform adapter (incremental, original PE preserved) */\n\tj2_pe_init_platform((void *)0);\n}\n", 1)

hdr = "#include <pexpert/pexpert.h>\n"
assert hdr in s, "header include not found"
s = s.replace(hdr, hdr + "\nextern void j2_pe_init_platform(void *);\n", 1)

# Append J2 platform adapter (UART + GIC + MCT) at end of file
s += '''

/* ---- J2 (Exynos 3475) platform adapter (incremental, appended) ---- */

/* UART0 base (verified via /proc/iomem on SM-J200M) */
#define J2_UART0_BASE     0x13800000U
#define J2_UART_ULCON     0x00U
#define J2_UART_UCON      0x04U
#define J2_UART_UFCON     0x08U
#define J2_UART_UBRDIV    0x28U
#define J2_UART_UDIVSLOT  0x2CU
#define J2_UART_UTRSTAT   0x10U
#define J2_UART_UTXH      0x20U
#define J2_UART_UTRSTAT_TX_EMPTY (1U << 2)

/* GIC-400 bases (VERIFIED via /proc/device-tree on SM-J200M)
 * Distributor: 0x10C01000, CPU Interface: 0x10C02000
 * Compatible: arm,cortex-a15-gic */
#define J2_GIC_DIST_BASE  0x10C01000U
#define J2_GIC_CPU_BASE   0x10C02000U
#define J2_GICD_CTLR      0x000U
#define J2_GICD_ISENABLER0 0x100U
#define J2_GICC_CTLR      0x000U
#define J2_GICC_PMR       0x004U
#define J2_GICD_CTLR_ENABLE 1U

/* MCT base (VERIFIED via /proc/device-tree on SM-J200M)
 * Frequency: 26 MHz (VERIFIED via /proc/device-tree/fixed-rate-clocks/oscclk/clock-frequency) */
#define J2_MCT_BASE       0x100B0000U
#define J2_MCT_CNT_L      0x100U
#define J2_MCT_CNT_H      0x104U

static void j2_uart_init(volatile unsigned int *base)
{
	base[J2_UART_ULCON / 4] = 0x03;   /* 8N1 */
	base[J2_UART_UCON / 4] = 0x05;    /* TX+RX, polling */
	base[J2_UART_UFCON / 4] = 0x00;   /* FIFO disable */
	base[J2_UART_UBRDIV / 4] = 26;    /* ~115200 @ 24MHz (UNVERIFIED clock) */
	base[J2_UART_UDIVSLOT / 4] = 0;
}

static void j2_gic_init(volatile unsigned int *dist, volatile unsigned int *cpu)
{
	dist[J2_GICD_CTLR / 4] = 0;
	dist[J2_GICD_CTLR / 4] = J2_GICD_CTLR_ENABLE;
	cpu[J2_GICC_PMR / 4] = 0xFF;
	cpu[J2_GICC_CTLR / 4] = 1;
}

static unsigned long long j2_mct_read_counter(volatile unsigned int *base)
{
	volatile unsigned int *cnt_l = base + (J2_MCT_CNT_L / 4);
	volatile unsigned int *cnt_u = base + (J2_MCT_CNT_H / 4);
	unsigned int hi1, lo, hi2;
	do {
		hi1 = *cnt_u;
		lo = *cnt_l;
		hi2 = *cnt_u;
	} while (hi1 != hi2);
	return ((unsigned long long)hi1 << 32) | lo;
}

void
j2_pe_init_platform(void *args)
{
	volatile unsigned int *uart = (volatile unsigned int *)J2_UART0_BASE;
	volatile unsigned int *gic_dist = (volatile unsigned int *)J2_GIC_DIST_BASE;
	volatile unsigned int *gic_cpu = (volatile unsigned int *)J2_GIC_CPU_BASE;
	volatile unsigned int *mct_base = (volatile unsigned int *)J2_MCT_BASE;
	unsigned long long mct_now;

	(void)args;

	j2_uart_init(uart);
	j2_gic_init(gic_dist, gic_cpu);
	mct_now = j2_mct_read_counter(mct_base);
	(void)mct_now;
}
'''

open(p, "w").write(s)
print("patched pe_init.c (J2 adapter: UART + GIC + MCT with dynamic counter read)")
PYEOF
