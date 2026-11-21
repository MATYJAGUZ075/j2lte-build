#ifndef J2_UART_H
#define J2_UART_H
#include <stdint.h>
/* Verified via /proc/iomem on SM-J200M */
#define J2_UART0_BASE 0x13800000U
#define J2_UART1_BASE 0x13810000U
#define J2_UART2_BASE 0x13820000U
#define J2_UART_REG_SIZE 0x100U
int j2_uart_base_valid(uint32_t base);
#endif
