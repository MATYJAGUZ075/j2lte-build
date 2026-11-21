#ifndef J2_SHIM_H
#define J2_SHIM_H
#include <stdint.h>
typedef struct { uint32_t physBase, virtBase, memSize, topOfKernelData; } j2_shim_memory_t;
int j2_shim_prepare(const j2_shim_memory_t *mem);
#endif
