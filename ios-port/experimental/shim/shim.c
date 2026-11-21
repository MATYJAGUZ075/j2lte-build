#include "shim.h"

int j2_shim_prepare(const j2_shim_memory_t *mem) {
    if (!mem) return 0;
    if (mem->physBase == 0 || mem->virtBase == 0) return 0;
    if (mem->topOfKernelData == 0) return 0;
    if (mem->topOfKernelData < mem->physBase) return 0;
    if (mem->memSize == 0) return 0;
    return 1;
}

int j2_shim_handoff_valid(const j2_shim_memory_t *mem) {
    if (!j2_shim_prepare(mem)) return 0;
    if (mem->dt_pa == 0 || mem->boot_args_pa == 0) return 0;
    if (mem->dt_pa < mem->physBase || mem->dt_pa >= mem->physBase + mem->memSize) return 0;
    if (mem->boot_args_pa < mem->physBase || mem->boot_args_pa >= mem->physBase + mem->memSize) return 0;
    if (mem->dt_pa == mem->boot_args_pa) return 0;
    if (mem->dt_pa + 4 > mem->physBase + mem->memSize) return 0;
    return 1;
}
