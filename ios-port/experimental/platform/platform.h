#ifndef J2_PLATFORM_H
#define J2_PLATFORM_H
#include <stdint.h>
#include "../shim/shim.h"

/* Platform init: validates handoff and initializes all early drivers.
   Returns 1 on success, 0 on failure. */
int j2_platform_init(const j2_shim_memory_t *mem);

/* Extended init with explicit MMIO bases (for host testing / bring-up).
   Returns number of components successfully initialized. */
int j2_platform_init_full(const j2_shim_memory_t *mem,
                          volatile uint32_t *uart,
                          volatile uint32_t *gic_dist,
                          volatile uint32_t *gic_cpu,
                          volatile uint32_t *mct);

#endif
