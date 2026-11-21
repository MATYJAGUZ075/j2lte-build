#include "shim.h"
int j2_shim_prepare(const j2_shim_memory_t *mem) {
    if (!mem || mem->topOfKernelData == 0) return 0;
    return 1;
}
