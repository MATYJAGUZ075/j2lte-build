#include "platform.h"
#include "../drivers/uart.h"
#include "../drivers/gic.h"
#include "../drivers/mct.h"

int j2_platform_init(const j2_shim_memory_t *mem) {
    if (!j2_shim_handoff_valid(mem)) return 0;
    return 1;
}

int j2_platform_init_full(const j2_shim_memory_t *mem,
                          volatile uint32_t *uart,
                          volatile uint32_t *gic_dist,
                          volatile uint32_t *gic_cpu,
                          volatile uint32_t *mct) {
    if (!j2_shim_handoff_valid(mem)) return 0;
    int ok = 0;
    if (uart) { j2_uart_init(uart); ok++; }
    if (gic_dist && gic_cpu) { j2_gic_init(gic_dist, gic_cpu); ok++; }
    if (mct) { /* MCT has no init write; just validate read */ ok++; }
    return ok;
}
