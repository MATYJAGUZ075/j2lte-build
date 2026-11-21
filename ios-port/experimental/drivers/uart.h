#ifndef J2_UART_H
#define J2_UART_H
#include <stdint.h>
/* Verified via /proc/iomem on SM-J200M */
#define J2_UART0_BASE 0x13800000U
#define J2_UART1_BASE 0x13810000U
#define J2_UART2_BASE 0x13820000U
#define J2_UART_REG_SIZE 0x100U

/* S3C64xx/Exynos UART registers (offsets) */
#define J2_UART_ULCON   0x00U
#define J2_UART_UCON    0x04U
#define J2_UART_UFCON   0x08U
#define J2_UART_UTRSTAT 0x10U
#define J2_UART_UERSTAT 0x14U
#define J2_UART_UFSTAT  0x18U
#define J2_UART_UTXH    0x20U
#define J2_UART_UBRDIV  0x28U
#define J2_UART_UDIVSLOT 0x2CU

#define J2_UART_UTRSTAT_TX_EMPTY (1U << 2)

int j2_uart_base_valid(uint32_t base);
/* host-testable: base points to a 0x100-byte register window */
void j2_uart_init(volatile uint32_t *base);
int j2_uart_putc(volatile uint32_t *base, char c, uint32_t timeout_iters);
#endif
