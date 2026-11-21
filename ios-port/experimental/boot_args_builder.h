#ifndef J2_BOOT_ARGS_BUILDER_H
#define J2_BOOT_ARGS_BUILDER_H

#include <stdint.h>
#include <stddef.h>

/* Mirrors pexpert/pexpert/arm/boot.h for ARM32. */
typedef struct {
    uint16_t Revision;
    uint16_t Version;
    uint32_t virtBase;
    uint32_t physBase;
    uint32_t memSize;
    uint32_t topOfKernelData;
    uint32_t v_baseAddr;
    uint32_t v_display;
    uint32_t v_rowBytes;
    uint32_t v_width;
    uint32_t v_height;
    uint32_t v_depth;
    uint32_t machineType;
    void *deviceTreeP;
    uint32_t deviceTreeLength;
    char CommandLine[256];
    uint32_t bootFlags;
    uint32_t memSizeActual;
} j2_boot_args_t;

typedef struct {
    uint32_t physBase;
    uint32_t virtBase;
    uint32_t memSize;
    uint32_t topOfKernelData;
    void *deviceTreeP;
    uint32_t deviceTreeLength;
    const char *commandLine;
    uint32_t machineType;
} j2_boot_args_inputs_t;

void j2_boot_args_init(j2_boot_args_t *out, const j2_boot_args_inputs_t *in);

#endif
