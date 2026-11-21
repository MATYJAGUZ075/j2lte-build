/*
 * J2 (Exynos 3475) Platform Expert adapter for XNU 4570.
 *
 * Provides PE_init_platform-like initialization using J2 early drivers.
 * Standalone version for CI compilation (XNU headers not available).
 */

#include <stdint.h>
#include "drivers/uart.h"
#include "drivers/gic.h"
#include "drivers/mct.h"
#include "platform/platform.h"

/* J2 early driver MMIO bases */
#define J2_UART0_BASE  0x13800000U
#define J2_GIC_BASE    0x10C01000U
#define J2_MCT_BASE    0x100B0000U

typedef struct {
    int initialized;
    void *deviceTreeHead;
    void *bootArgs;
} j2_pe_state_t;

static j2_pe_state_t j2_pe_state = {0, 0, 0};

int j2_pe_init_platform(int vm_initialized, void *args) {
    if (!j2_pe_state.initialized) {
        j2_pe_state.initialized = 1;
        j2_pe_state.bootArgs = args;
    }
    if (!vm_initialized) {
        /* Early platform init: DT, machine identification */
    } else {
        /* Interrupts and debug init */
    }
    return 1;
}

int j2_pe_state_initialized(void) { return j2_pe_state.initialized; }
