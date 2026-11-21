#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../macho/macho_loader.h"
#include "../macho/full/macho_full.h"
#include "../memory/memory_manager.h"
#include "../boot_args_builder.h"
#include "../apple_dt_serializer.h"
#include "../shim/shim.h"
#include "../boot_args_validator.h"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } else { printf("PASS: %s\n", msg); } } while (0)

int main(void) {
    uint8_t macho[28 + 56] = {0};
    uint32_t *w = (uint32_t*)macho;
    w[0] = 0xFEEDFACEU;
    w[1] = 12;
    w[2] = 12;
    w[4] = 1;
    uint32_t *lc = (uint32_t*)(macho + 28);
    lc[0] = 1;
    lc[1] = 56;
    memcpy(macho + 28 + 8, "__TEXT", 6);
    uint32_t *seg = (uint32_t*)(macho + 28 + 24);
    seg[0] = 0x80100000;
    seg[1] = 0x003A0000;
    seg[2] = 0;
    seg[3] = 0x003A0000;

    CHECK(j2_macho_validate_header(macho, sizeof(macho)) == 1, "macho header valid");

    j2_segment_t segs[4]; size_t nseg = 0;
    CHECK(j2_parse_segments(macho, sizeof(macho), segs, 4, &nseg) == 1, "parse segments");
    CHECK(nseg == 1, "one segment");
    CHECK(segs[0].vmaddr == 0x80100000, "segment vmaddr");

    CHECK(j2_memory_validate_region(0x80100000, 0x003A0000, 0x80000000, 0xFFFEFFFF) == 1, "segment in kernel range");
    CHECK(j2_memory_validate_region(0x20000000, 0x3F500000, 0x20000000, 0x5F500000) == 1, "ram range");

    j2_boot_args_t ba;
    j2_boot_args_inputs_t in = {0};
    in.physBase = 0x20000000;
    in.virtBase = 0x80000000;
    in.memSize = 0x3F500000;
    in.topOfKernelData = 0x80536000;
    in.machineType = 0x20;
    in.commandLine = "rd=md0";
    j2_boot_args_init(&ba, &in);
    CHECK(ba.physBase == 0x20000000, "boot_args physBase");
    CHECK(ba.topOfKernelData == 0x80536000, "boot_args topOfKernelData");

    CHECK(j2_dt_align4(5) == 8, "dt align");
    j2_dt_node_header_t node = {0};
    size_t sz = j2_dt_node_size(&node, 4, 0);
    CHECK(sz >= sizeof(j2_dt_node_header_t), "dt node size");

    j2_shim_memory_t mem = { in.physBase, in.virtBase, in.memSize, in.topOfKernelData };
    CHECK(j2_shim_prepare(&mem) == 1, "shim prepare");

    CHECK(j2_boot_args_validate(in.physBase, in.virtBase, in.memSize, in.topOfKernelData) == 1, "boot_args validate");

    if (failures == 0) {
        printf("ALL INTEGRATION CHECKS PASSED\n");
        return 0;
    }
    printf("%d FAILURES\n", failures);
    return 1;
}
EOF
Path: D:\J2_XNU_PORT\repo-j2lte-build\ios-port\experimental\integration\integration_test.c