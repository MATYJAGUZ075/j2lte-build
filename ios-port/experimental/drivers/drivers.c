#include "drivers/uart.h"
#include "drivers/gic.h"
#include "drivers/mct.h"

int j2_uart_base_valid(uint32_t base) {
    return base == J2_UART0_BASE || base == J2_UART1_BASE || base == J2_UART2_BASE;
}
int j2_gic_base_valid(uint32_t base) { return base == J2_GIC_BASE; }
int j2_mct_base_valid(uint32_t base) { return base == J2_MCT_BASE; }
