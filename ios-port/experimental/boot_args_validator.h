#ifndef J2_BOOT_ARGS_VALIDATOR_H
#define J2_BOOT_ARGS_VALIDATOR_H
#include <stdint.h>
int j2_boot_args_validate(uint32_t physBase, uint32_t virtBase, uint32_t memSize, uint32_t topOfKernelData);
#endif
