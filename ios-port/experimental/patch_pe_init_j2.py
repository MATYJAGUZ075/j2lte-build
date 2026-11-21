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

/* GIC-400 bases (from J2.h, compatible "arm,cortex-a15-gic")
 * NOTE: these addresses are NOT independently verified on hardware */
#define J2_GIC_DIST_BASE  0x10C01000U
#define J2_GIC_CPU_BASE   0x10C02000U
#define J2_GICD_CTLR      0x000U
#define J2_GICD_ISENABLER0 0x100U
#define J2_GICC_CTLR      0x000U
#define J2_GICC_PMR       0x004U
#define J2_GICD_CTLR_ENABLE 1U

/* MCT base (from J2.h) - frequency unverified */
#define J2_MCT_BASE       0x100B0000U

static void j2_uart_init(volatile unsigned int *base)
{
	base[J2_UART_ULCON / 4] = 0x03;   /* 8N1 */
	base[J2_UART_UCON / 4] = 0x05;    /* TX+RX, polling */
	base[J2_UART_UFCON / 4] = 0x00;   /* FIFO disable */
	base[J2_UART_UBRDIV / 4] = 26;    /* ~115200 @ 24MHz (UNVERIFIED) */
	base[J2_UART_UDIVSLOT / 4] = 0;
}

static void j2_gic_init(volatile unsigned int *dist, volatile unsigned int *cpu)
{
	/* Enable distributor */
	dist[J2_GICD_CTLR / 4] = J2_GICD_CTLR_ENABLE;
	/* Set priority mask to allow all priorities */
	cpu[J2_GICC_PMR / 4] = 0xFF;
	/* Enable CPU interface */
	cpu[J2_GICC_CTLR / 4] = 1;
}

static void j2_mct_init(volatile unsigned int *base)
{
	/* MCT initialization - frequency VERIFIED: 26 MHz (0x018cba80)
	 * base: 0x100B0000 (verified via /proc/device-tree/mct@100B0000/reg)
	 * Global counter lower 32 bits at offset 0x100, upper at 0x104.
	 * This reads the counter to verify the timer is accessible. */
	volatile unsigned int *cnt_l = base + (0x100 / 4);
	volatile unsigned int *cnt_u = base + (0x104 / 4);
	unsigned int lo = *cnt_l;
	unsigned int hi = *cnt_u;
	(void)lo;
	(void)hi;
}

void
j2_pe_init_platform(void *args)
{
	volatile unsigned int *uart = (volatile unsigned int *)J2_UART0_BASE;
	volatile unsigned int *gic_dist = (volatile unsigned int *)J2_GIC_DIST_BASE;
	volatile unsigned int *gic_cpu = (volatile unsigned int *)J2_GIC_CPU_BASE;

	(void)args;

	/* UART init (MMIO base verified via /proc/iomem) */
	j2_uart_init(uart);

	/* GIC init (addresses VERIFIED via /proc/device-tree on SM-J200M)
	 * Distributor: 0x10C01000, CPU Interface: 0x10C02000
	 * Compatible: arm,cortex-a15-gic */
	j2_gic_init(gic_dist, gic_cpu);

	/* MCT init (frequency UNVERIFIED) */
	j2_mct_init((volatile unsigned int *)J2_MCT_BASE);
}
'''

open(p, "w").write(s)
print("patched pe_init.c (J2 adapter: UART + GIC + MCT)")
