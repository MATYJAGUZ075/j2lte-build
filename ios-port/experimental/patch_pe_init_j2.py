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

# Append UART driver + adapter implementation at end of file
s += '''

/* ---- J2 UART driver (incremental, appended) ---- */
#define J2_UART0_BASE 0x13800000U
#define J2_UART_ULCON   0x00U
#define J2_UART_UCON    0x04U
#define J2_UART_UFCON   0x08U
#define J2_UART_UBRDIV  0x28U
#define J2_UART_UDIVSLOT 0x2CU
#define J2_UART_UTRSTAT 0x10U
#define J2_UART_UTXH    0x20U
#define J2_UART_UTRSTAT_TX_EMPTY (1U << 2)

static void j2_uart_init(volatile unsigned int *base)
{
	base[J2_UART_ULCON / 4] = 0x03;   /* 8N1 */
	base[J2_UART_UCON / 4] = 0x05;    /* TX+RX, polling */
	base[J2_UART_UFCON / 4] = 0x00;   /* FIFO disable */
	base[J2_UART_UBRDIV / 4] = 26;    /* ~115200 @ 24MHz (unverified) */
	base[J2_UART_UDIVSLOT / 4] = 0;
}

static int j2_uart_putc(volatile unsigned int *base, char c, unsigned int timeout)
{
	unsigned int i;
	for (i = 0; i < timeout; i++) {
		if (base[J2_UART_UTRSTAT / 4] & J2_UART_UTRSTAT_TX_EMPTY) {
			base[J2_UART_UTXH / 4] = (unsigned int)c;
			return 0;
		}
	}
	return -1;
}

void
j2_pe_init_platform(void *args)
{
	volatile unsigned int *uart = (volatile unsigned int *)J2_UART0_BASE;
	(void)args;
	j2_uart_init(uart);
	/* Send a single marker byte to verify TX path (not proven on hardware) */
	j2_uart_putc(uart, 'J', 100000);
}
'''

open(p, "w").write(s)
print("patched pe_init.c (J2 adapter + UART init)")
PYEOF
