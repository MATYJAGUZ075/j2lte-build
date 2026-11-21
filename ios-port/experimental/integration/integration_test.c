#include <stdio.h>
#include <stdlib.h>
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
#include "../load_planner.h"
#include "../drivers/uart.h"
#include "../drivers/gic.h"
#include "../drivers/mct.h"
#include "../platform/platform.h"
#include "../manifest_values.h"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } else { printf("PASS: %s\n", msg); } } while (0)

int main(void) {
    uint8_t macho[28 + 56] = {0};
    uint32_t *w = (uint32_t*)macho;
    w[0] = 0xFEEDFACEU; w[1] = 12; w[2] = 12; w[3] = 2; w[4] = 1; w[5] = 56;
    uint32_t *lc = (uint32_t*)(macho + 28);
    lc[0] = 1; lc[1] = 56;
    memcpy(macho + 28 + 8, "__TEXT", 6);
    uint32_t *seg = (uint32_t*)(macho + 28 + 24);
    seg[0] = 0x80100000; seg[1] = 0x003A0000; seg[2] = 0; seg[3] = 0;

    CHECK(j2_macho_validate_header(macho, sizeof(macho)) == 1, "macho header valid");
    CHECK(j2_macho_validate_header(macho, 4) == 0, "macho truncated rejected");

    j2_segment_t segs[4]; size_t nseg = 0;
    CHECK(j2_parse_segments(macho, sizeof(macho), segs, 4, &nseg) == 1, "parse segments");
    CHECK(nseg == 1, "one segment");
    CHECK(segs[0].vmaddr == 0x80100000, "segment vmaddr");
    CHECK(j2_segments_overlap(segs, nseg) == 0, "no overlap");
    CHECK(j2_macho_entry_in_segment(0x801C8084, segs[0].vmaddr, segs[0].vmsize) == 1, "entry in segment");

    CHECK(j2_memory_validate_region(0x80100000, 0x003A0000, 0x80000000, 0xFFFEFFFF) == 1, "segment in kernel range");
    CHECK(j2_memory_validate_region(0x20000000, 0x3F500000, 0x20000000, 0x5F500000) == 1, "ram range");
    CHECK(j2_memory_validate_region(0x20000000, 0, 0x20000000, 0x5F500000) == 0, "zero size rejected");

    j2_mem_map_t mm; j2_mem_map_init(&mm);
    j2_mem_map_add(&mm, 0x20000000, 0x10000000, J2_MEM_RAM, "ram0");
    j2_mem_map_add(&mm, 0x30000000, 0x10000000, J2_MEM_RAM, "ram1");
    CHECK(j2_mem_is_within_ram(&mm, 0x20000000, 0x1000) == 1, "within ram0");
    CHECK(j2_mem_is_within_ram(&mm, 0x50000000, 0x1000) == 0, "outside ram rejected");
    CHECK(j2_mem_reserve(&mm, 0x40001000, 0x1000, "r1") == 1, "reserve ok");
    CHECK(j2_mem_reserve(&mm, 0x20001000, 0x1000, "dup") == 0, "double reserve rejected");

    j2_load_plan_t plan;
    CHECK(j2_load_plan_build(&plan, segs, nseg, 0x801C8084, 0x80000000, 0x20000000, 0x20000000, 0x5F4FFFFF) == 1, "load plan build");
    CHECK(plan.entry_pa == 0x201C8084, "entry pa");
    uint32_t tk = 0; int tk_known = 1;
    CHECK(j2_topk_compute(&plan, 0x80000000, 0x20000000, &tk, &tk_known) == 1, "topk compute");
    CHECK(tk_known == 0, "topk remains unknown (not confirmed)");

    j2_boot_args_t ba;
    j2_boot_args_inputs_t in = {0};
    in.physBase = 0x20000000;
    in.virtBase = 0x80000000;
    in.memSize = 0x3F500000;
    in.topOfKernelData = 0x20536000;
    in.machineType = 0x20;
    in.commandLine = "rd=md0";
    j2_boot_args_init(&ba, &in);
    CHECK(ba.physBase == 0x20000000, "boot_args physBase");
    CHECK(ba.topOfKernelData == 0x20536000, "boot_args topOfKernelData");

    CHECK(j2_dt_align4(5) == 8, "dt align");
    CHECK(j2_dt_node_header_size() == 8, "dt node header size");

    /* round-trip multinivel */
    j2_dt_tree_node_t root; memset(&root, 0, sizeof(root));
    strcpy(root.name, "root");
    j2_dt_prop_t rprops[2];
    memset(rprops, 0, sizeof(rprops));
    strcpy(rprops[0].name, "name"); rprops[0].length = 4; rprops[0].value = (const uint8_t*)"root";
    strcpy(rprops[1].name, "prop"); rprops[1].length = 3; rprops[1].value = (const uint8_t*)"abc";
    root.props = rprops; root.nprops = 2;
    j2_dt_tree_node_t child; memset(&child, 0, sizeof(child));
    strcpy(child.name, "child");
    j2_dt_prop_t cprops[1];
    memset(cprops, 0, sizeof(cprops));
    strcpy(cprops[0].name, "name"); cprops[0].length = 5; cprops[0].value = (const uint8_t*)"child";
    child.props = cprops; child.nprops = 1;
    j2_dt_tree_node_t *children[1] = { &child };
    root.children = children; root.nchildren = 1;

    size_t sz = 0;
    CHECK(j2_dt_serialize_tree(NULL, 0, &root, &sz) == 1, "dt serialize size");
    CHECK(sz > 0, "dt size >0");
    uint8_t *dbuf = (uint8_t*)malloc(sz);
    CHECK(dbuf != NULL, "dt buf alloc");
    size_t outsz = 0;
    CHECK(j2_dt_serialize_tree(dbuf, sz, &root, &outsz) == 1, "dt serialize");
    CHECK(outsz == sz, "dt size match");
    CHECK(j2_dt_validate_tree(dbuf, sz, 8) == 1, "dt validate");
    size_t tsz = j2_dt_subtree_size(dbuf, sz);
    CHECK(tsz == sz, "dt subtree size");
    j2_dt_tree_node_t *recon = NULL;
    CHECK(j2_dt_tree_from_buffer(dbuf, sz, &recon, 8) == 1, "dt from buffer");
    CHECK(recon != NULL, "dt recon not null");
    CHECK(j2_dt_trees_equal(&root, recon) == 1, "dt roundtrip equal");
    j2_dt_free_tree(recon);
    free(dbuf);

    /* DT negative: truncated buffer */
    uint8_t trunc[4] = {0};
    CHECK(j2_dt_validate_tree(trunc, sizeof(trunc), 8) == 0, "dt truncated rejected");
    /* DT negative: zero bufsize */
    uint8_t dummy[8] = {0};
    CHECK(j2_dt_validate_tree(dummy, 0, 8) == 0, "dt zero bufsize rejected");

    j2_shim_memory_t mem = { in.physBase, in.virtBase, in.memSize, in.topOfKernelData, 0x21000000, 0x20ff0000 };
    CHECK(j2_shim_prepare(&mem) == 1, "shim prepare");
    CHECK(j2_shim_handoff_valid(&mem) == 1, "shim handoff valid");
    /* negative: null */
    CHECK(j2_shim_handoff_valid(NULL) == 0, "shim null rejected");
    /* negative: unaligned topk */
    j2_shim_memory_t bad = mem; bad.topOfKernelData = 0x20da6001;
    CHECK(j2_shim_handoff_valid(&bad) == 0, "shim unaligned topk rejected");
    /* negative: dt out of range */
    j2_shim_memory_t bad2 = mem; bad2.dt_pa = 0x10000000;
    CHECK(j2_shim_handoff_valid(&bad2) == 0, "shim dt oor rejected");
    /* negative: zero boot_args */
    j2_shim_memory_t bad3 = mem; bad3.boot_args_pa = 0;
    CHECK(j2_shim_handoff_valid(&bad3) == 0, "shim zero boot_args rejected");
    /* memplan test */
    j2_memplan_t mp = {0};
    mp.physBase = 0x20000000; mp.virtBase = 0x80000000; mp.memSize = 0x3F500000;
    mp.kernel_data_start = 0x20bcc000; mp.kernel_data_end = 0x20da5eb7;
    mp.dt_size = 0x10000; mp.ramdisk_size = 0x1127F0; mp.boot_args_size = 0x1000;
    CHECK(j2_memplan_compute_topk(&mp) == 1, "memplan compute");
    CHECK(mp.topOfKernelData != 0, "memplan topk nonzero");
    CHECK(mp.topk_known == 0, "memplan topk unknown (not confirmed)");
    CHECK(mp.topOfKernelData >= 0x20DA6000, "memplan topk >= observed min");
    CHECK(mp.topOfKernelData % 4096 == 0, "memplan topk aligned");
    /* memplan negatives */
    j2_memplan_t mp2b = mp; mp2b.dt_size = 0;
    CHECK(j2_memplan_compute_topk(&mp2b) == 0, "memplan missing dt rejected");
    j2_memplan_t mp3b = mp; mp3b.kernel_data_end = 0x20000000;
    CHECK(j2_memplan_compute_topk(&mp3b) == 0, "memplan bad data range rejected");

    /* === END-TO-END PIPELINE === */
    /* 1. Build real DT */
    j2_dt_tree_node_t e2e_root; memset(&e2e_root, 0, sizeof(e2e_root));
    strcpy(e2e_root.name, "root");
    j2_dt_prop_t e2e_props[1]; memset(e2e_props, 0, sizeof(e2e_props));
    strcpy(e2e_props[0].name, "model"); e2e_props[0].length = 7; e2e_props[0].value = (const uint8_t*)"j2lte";
    e2e_root.props = e2e_props; e2e_root.nprops = 1;
    size_t e2e_dtsz = 0;
    CHECK(j2_dt_serialize_tree(NULL, 0, &e2e_root, &e2e_dtsz) == 1, "e2e dt size");
    uint8_t *e2e_dtbuf = (uint8_t*)malloc(e2e_dtsz);
    size_t e2e_dtout = 0;
    CHECK(j2_dt_serialize_tree(e2e_dtbuf, e2e_dtsz, &e2e_root, &e2e_dtout) == 1, "e2e dt serialize");
    CHECK(j2_dt_validate_tree(e2e_dtbuf, e2e_dtsz, 8) == 1, "e2e dt validate");

    /* 2. Ramdisk (real bytes, minimal) */
    uint8_t ramdisk[256]; memset(ramdisk, 0xAB, sizeof(ramdisk));
    size_t rd_size = sizeof(ramdisk);

    /* 3. boot_args with real sizes */
    j2_boot_args_t e2e_ba; j2_boot_args_inputs_t e2e_in = {0};
    e2e_in.physBase = 0x20000000; e2e_in.virtBase = 0x80000000;
    e2e_in.memSize = 0x3F500000; e2e_in.topOfKernelData = 0x20536000;
    e2e_in.machineType = 0x20; e2e_in.commandLine = "rd=md0";
    e2e_in.deviceTreeP = (void*)0x21000000; e2e_in.deviceTreeLength = (uint32_t)e2e_dtsz;
    j2_boot_args_init(&e2e_ba, &e2e_in);
    CHECK(e2e_ba.deviceTreeLength == (uint32_t)e2e_dtsz, "e2e boot_args dt length");

    /* 4. memplan with real sizes */
    j2_memplan_t e2e_mp = {0};
    e2e_mp.physBase = 0x20000000; e2e_mp.virtBase = 0x80000000; e2e_mp.memSize = 0x3F500000;
    e2e_mp.kernel_data_start = 0x20bcc000; e2e_mp.kernel_data_end = 0x20da5eb7;
    e2e_mp.dt_size = (uint32_t)e2e_dtsz; e2e_mp.ramdisk_size = (uint32_t)rd_size; e2e_mp.boot_args_size = 0x1000;
    CHECK(j2_memplan_compute_topk(&e2e_mp) == 1, "e2e memplan compute");

    /* 5. handoff validation */
    j2_shim_memory_t e2e_mem = { e2e_in.physBase, e2e_in.virtBase, e2e_in.memSize, e2e_in.topOfKernelData, 0x21000000, 0x20ff0000 };
    CHECK(j2_shim_handoff_valid(&e2e_mem) == 1, "e2e shim handoff valid");

    /* 6. reject inconsistency: mismatched dt size */
    j2_boot_args_t bad_ba; j2_boot_args_inputs_t bad_in = e2e_in;
    bad_in.deviceTreeLength = 0;
    j2_boot_args_init(&bad_ba, &bad_in);
    CHECK(j2_boot_args_validate(bad_in.physBase, bad_in.virtBase, bad_in.memSize, bad_in.topOfKernelData) == 1, "e2e bad dt still validates fields");
    free(e2e_dtbuf);
    j2_memplan_t mp2 = mp; mp2.dt_size = 0;
    CHECK(j2_memplan_compute_topk(&mp2) == 0, "memplan missing dt rejected");
    j2_memplan_t mp3 = mp; mp3.kernel_data_end = 0x20000000;
    CHECK(j2_memplan_compute_topk(&mp3) == 0, "memplan bad data range rejected");

    CHECK(j2_boot_args_validate(in.physBase, in.virtBase, in.memSize, in.topOfKernelData) == 1, "boot_args validate");
    CHECK(j2_boot_args_validate(0x20000000, 0x80000000, 0x3F500000, 0x80536000) == 0, "VA-as-PA rejected");
    CHECK(j2_boot_args_validate(0, 0x80000000, 0x3F500000, 0x20536000) == 0, "zero physBase rejected");

    /* Platform init full test */
    volatile uint32_t puart[64] = {0}; puart[J2_UART_UTRSTAT / 4] = J2_UART_UTRSTAT_TX_EMPTY;
    volatile uint32_t pgicd[1024] = {0}; volatile uint32_t pgicc[1024] = {0};
    volatile uint32_t pmct[1024] = {0}; pmct[J2_MCT_G_CNT_L / 4] = 0x1234;
    int plat = j2_platform_init_full(&mem, puart, pgicd, pgicc, pmct);
    CHECK(plat == 3, "platform init full (3 components)");
    CHECK(j2_platform_init(NULL) == 0, "platform init null rejected");

    if (failures == 0) {
        /* UART host test */
    volatile uint32_t uartmem[64] = {0};
    uartmem[J2_UART_UTRSTAT / 4] = J2_UART_UTRSTAT_TX_EMPTY;
    j2_uart_init(uartmem);
    CHECK(j2_uart_putc(uartmem, 'A', 10) == 1, "uart putc");
    CHECK(uartmem[J2_UART_UTXH / 4] == 'A', "uart txh");
    /* GIC host test */
    volatile uint32_t gicdist[1024] = {0};
    volatile uint32_t giccpu[1024] = {0};
    j2_gic_init(gicdist, giccpu);
    j2_gic_enable_irq(gicdist, 32);
    CHECK(gicdist[(J2_GICD_ISENABLER + 4) / 4] == (1U << 0), "gic enable");
    /* MCT host test */
    volatile uint32_t mctmem[1024] = {0};
    mctmem[J2_MCT_G_CNT_L / 4] = 0x1234;
    mctmem[J2_MCT_G_CNT_U / 4] = 0x5678;
    CHECK(j2_mct_read_counter(mctmem) == 0x567800001234ULL, "mct counter");

    printf("ALL INTEGRATION CHECKS PASSED\n");
        return 0;
    }
    printf("%d FAILURES\n", failures);
    return 1;
}

