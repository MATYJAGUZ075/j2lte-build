#ifndef J2_SHIM_H
#define J2_SHIM_H
#include <stdint.h>
typedef struct { uint32_t physBase, virtBase, memSize, topOfKernelData; uint32_t dt_pa, boot_args_pa; } j2_shim_memory_t;
int j2_shim_prepare(const j2_shim_memory_t *mem);
int j2_shim_handoff_valid(const j2_shim_memory_t *mem);
#endif
