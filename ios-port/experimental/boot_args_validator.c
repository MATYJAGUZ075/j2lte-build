#include "boot_args_validator.h"
int j2_boot_args_validate(uint32_t physBase, uint32_t virtBase, uint32_t memSize, uint32_t topOfKernelData) {
    if (physBase == 0) return 0;
    if (virtBase < 0x80000000U) return 0;
    if (memSize == 0) return 0;
    if (topOfKernelData == 0) return 0;
    if (topOfKernelData < physBase) return 0;
    return 1;
}

