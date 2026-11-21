#include "boot_args_builder.h"
#include <string.h>

void j2_boot_args_init(j2_boot_args_t *out, const j2_boot_args_inputs_t *in) {
    if (!out || !in) return;
    memset(out, 0, sizeof(*out));
    out->Revision = 1;
    out->Version = 2;
    out->virtBase = in->virtBase;
    out->physBase = in->physBase;
    out->memSize = in->memSize;
    out->topOfKernelData = in->topOfKernelData;
    out->machineType = in->machineType;
    out->deviceTreeP = in->deviceTreeP;
    out->deviceTreeLength = in->deviceTreeLength;
    out->memSizeActual = in->memSize;
    if (in->commandLine) {
        strncpy(out->CommandLine, in->commandLine, sizeof(out->CommandLine) - 1);
    }
}
