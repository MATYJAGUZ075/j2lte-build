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

int j2_uart_getc(volatile uint32_t *base, uint32_t timeout_iters) {
    if (!base) return -1;
    uint32_t i;
    for (i = 0; i < timeout_iters; i++) {
        if (base[J2_UART_UTRSTAT / 4] & J2_UART_UTRSTAT_RX_READY) {
            return (int)(base[J2_UART_URXH / 4] & 0xFFU);
        }
    }
    return -1;
}

void j2_uart_set_baud(volatile uint32_t *base, uint32_t div, uint32_t slot) {
    if (!base) return;
    base[J2_UART_UBRDIV / 4] = div;
    base[J2_UART_UDIVSLOT / 4] = slot;
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

void j2_gic_set_priority(volatile uint32_t *dist, uint32_t irq, uint8_t prio) {
    if (!dist) return;
    dist[(J2_GICD_IPRIORITYR + irq) / 4] = prio;
}

uint32_t j2_gic_get_priority(volatile uint32_t *dist, uint32_t irq) {
    if (!dist) return 0;
    return dist[(J2_GICD_IPRIORITYR + irq) / 4];
}

int j2_mct_base_valid(uint32_t base) { return base == J2_MCT_BASE; }

uint64_t j2_mct_read_counter(volatile uint32_t *base) {
    if (!base) return 0;
    uint32_t hi = base[J2_MCT_G_CNT_U / 4];
    uint32_t lo = base[J2_MCT_G_CNT_L / 4];
    return ((uint64_t)hi << 32) | lo;
}

uint64_t j2_mct_ticks_to_ns(uint64_t ticks) {
    return ticks * 1000000000ULL / J2_MCT_FREQ_HZ;
}

uint64_t j2_mct_ns_to_ticks(uint64_t ns) {
    return ns * J2_MCT_FREQ_HZ / 1000000000ULL;
}
