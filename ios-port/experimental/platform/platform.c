#include "platform.h"
#include "../drivers/uart.h"
#include "../drivers/gic.h"
#include "../drivers/mct.h"

int j2_platform_init(const j2_shim_memory_t *mem) {
    if (!j2_shim_handoff_valid(mem)) return 0;
    /* Early UART for diagnostics */
    /* GIC init placeholder: requires MMIO base; validated only */
    /* MCT init placeholder */
    return 1;
}
