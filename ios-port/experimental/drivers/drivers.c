#include "drivers/uart.h"
#include "drivers/gic.h"
#include "drivers/mct.h"

int j2_uart_base_valid(uint32_t base) {
    return base == J2_UART0_BASE || base == J2_UART1_BASE || base == J2_UART2_BASE;
}

void j2_uart_init(volatile uint32_t *base) {
    if (!base) return;
    base[J2_UART_ULCON / 4] = 0x03;
    base[J2_UART_UCON / 4] = 0x05;
    base[J2_UART_UFCON / 4] = 0x01;
    base[J2_UART_UBRDIV / 4] = 26;
    base[J2_UART_UDIVSLOT / 4] = 0x0000;
}

int j2_uart_putc(volatile uint32_t *base, char c, uint32_t timeout_iters) {
    if (!base) return 0;
    uint32_t i;
    for (i = 0; i < timeout_iters; i++) {
        if (base[J2_UART_UTRSTAT / 4] & J2_UART_UTRSTAT_TX_EMPTY) {
            base[J2_UART_UTXH / 4] = (uint32_t)c;
            return 1;
        }
    }
    return 0;
}

int j2_gic_base_valid(uint32_t base) { return base == J2_GIC_BASE; }

void j2_gic_init(volatile uint32_t *dist, volatile uint32_t *cpu) {
    if (!dist || !cpu) return;
    dist[J2_GICD_CTLR / 4] = 0; /* disable */
    dist[J2_GICD_CTLR / 4] = J2_GICD_CTLR_ENABLE;
    cpu[J2_GICC_PMR / 4] = 0xFF; /* allow all priorities */
    cpu[J2_GICC_CTLR / 4] = 1;    /* enable */
}

void j2_gic_enable_irq(volatile uint32_t *dist, uint32_t irq) {
    if (!dist) return;
    dist[(J2_GICD_ISENABLER + (irq / 32) * 4) / 4] = (1U << (irq % 32));
}

void j2_gic_disable_irq(volatile uint32_t *dist, uint32_t irq) {
    if (!dist) return;
    dist[(J2_GICD_ICENABLER + (irq / 32) * 4) / 4] = (1U << (irq % 32));
}

uint32_t j2_gic_ack(volatile uint32_t *cpu) {
    if (!cpu) return 0;
    return cpu[J2_GICC_IAR / 4];
}

void j2_gic_eoi(volatile uint32_t *cpu, uint32_t iar) {
    if (!cpu) return;
    cpu[J2_GICC_EOIR / 4] = iar;
}

int j2_mct_base_valid(uint32_t base) { return base == J2_MCT_BASE; }

uint64_t j2_mct_read_counter(volatile uint32_t *base) {
    if (!base) return 0;
    uint32_t hi = base[J2_MCT_G_CNT_U / 4];
    uint32_t lo = base[J2_MCT_G_CNT_L / 4];
    return ((uint64_t)hi << 32) | lo;
}
