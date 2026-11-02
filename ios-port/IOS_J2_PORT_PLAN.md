# PLAN DE PORT MÍNIMO iOS 10.3.4 EN SAMSUNG GALAXY J2 (SM-J200M)

## Objetivo
**Manzanita + video (framebuffer) + touch** en J2.
- Kernel: XNU 4570.71.2 (iOS 10.3) ARMv7
- Userland: iPhone 5,1 iOS 10.3.4 (14G61) — único 32-bit Apple
- Boot: S-Boot → loader en partición BOOT (13 MB) → XNU → HFS+ root en USERDATA (eMMC)
- Video: DECON framebuffer heredado de S-Boot, render CPU (CoreAnimation software)
- Touch: zinitix (I2C) + sec_touchkey (GPIO)

## Restricciones Hard
| Ítem | Valor | Implicación |
|---|---|---|
| BOOT partition | 13,631,488 bytes (13 MB) | Kernel + loader + ADT + ramdisk ≤ 13 MB |
| Recovery | Ocupado por TWRP | **No tocar**. Usar BOOT o USERDATA |
| RAM usable | 814-880 MB | Shared cache 1.2 GB DEBE ir en eMMC |
| Bootloader | S-Boot (Odin/TWRP only) | No `fastboot boot`. Flasheo obligatorio BOOT |
| Debug | Sin UART, sin pstore | Solo framebuffer DECON o jig 619K |

## Fases del Plan

### FASE 0: Validación Enlace Kernel XNU 4570 ARMv7 (2-4 sem, SOLO CI)
**Objetivo:** Confirmar que XNU 4570.71.2 enlaza completo con `MACHINE_CONFIG=J2`.

**Pasos:**
1. Clonar `apple-oss-distributions/xnu` tag `xnu-4570.71.2`
2. Aplicar patches base de `mt6580-xnu`:
   - `makedefs/MakeInc.def` (ARM arch/machine flags, linker, dsymutil)
   - `makedefs/MakeInc.top` (default MACHINE_CONFIG=J2)
   - `config/MASTER.arm` (`options CONFIG_EMBEDDED`)
   - `pexpert/pexpert/arm/board_config.h` (`ARM_BOARD_CONFIG_J2`)
   - `pexpert/pexpert/arm/J2.h` (direcciones J2: GIC, MCT, memoria, DECON, dwmmc, I2C, PMIC, RTC)
   - `bsd/kern/makekdebugevents.py` (Python 3 compat)
   - Fixes: firehose compat, VFP2 vmrs, lz4 C, static PIE, armv7ve asm, banked MRS
3. Crear workflow CI macOS en `j2lte-build` (rama local `ios-port-fase0`)
4. Build: `make -C xnu TARGET_CONFIGS="RELEASE ARM J2" ...`
5. **Criterio éxito:** Enlace sin errores. Si solo `dlil.c` alineación 8-byte → fix trivial. Si >5 errores → reevaluar.

**Entregable:** Log de build CI + lista exacta de errores (si los hay).

---

### FASE 1: Shim/Loader para S-Boot en BOOT Partition (4-6 sem)
**Objetivo:** Loader ARMv7 que S-Boot cargue desde partición BOOT (13 MB).

**Componentes:**
| Componente | Tamaño estimado | Notas |
|---|---|---|
| Loader bare metal (start.S, mmu.c, fdt.c, console.c) | ~200-400 KB | ARMv7, 4 KB pages, identity map |
| XNU kernel (Mach-O, static) | 8-12 MB | Debe caber + ADT + ramdisk |
| ADT generado | ~50-100 KB | Desde live DTB S-Boot |
| Ramdisk mínimo (init hello) | 2-4 MB | Solo para early console |
| **Total** | **11-16 MB** | **Límite 13 MB** |

**Estrategia si overflow:**
- Ramdisk ultra-mínimo (init 500 KB) + comprimir kernel (S-Boot no descomprime Mach-O raw)
- O usar RECOVERY (15 MB) SOLO si usuario autoriza (tiene TWRP ahí)

**Pasos detallados:**
1. **Extraer info S-Boot stock:**
   - `boot.img` stock → parsear zImage-dtb, DTB appended, `chosen/bootargs`
   - DTB live desde `/sys/firmware/fdt` (Android/TWRP)
   - Dirección framebuffer DECON dejada por S-Boot (buscar en DTB `decon_fb@14830000` `reg`)
2. **Portar `ios-sweet/shim/` a ARMv7:**
   - `start.S`: entry ARMv7, setup MMU 4 KB, jump a C
   - `mmu.c`: identity map 1:1 (kernel en RAM física), page tables 4 KB
   - `fdt.c`: parse DTB appended por S-Boot, extraer memory, GIC, MCT, DECON, dwmmc, I2C, PMIC, RTC
   - `console.c`: early printk a DECON FB (dirección desde DTB)
3. **Generar ADT J2 (`tools/mkadt_j2.py`):**
   - `/cpus`: 4× `arm,cortex-a7` MPIDR 0,1,2,3
   - `/arm-io` children:
     - `gic`: `arm,cortex-a15-gic` @0x10C01000/0x10C02000
     - `mct`: `samsung,exynos4210-mct` @0x100B0000, interrupts 102-113
     - `decon`: `samsung,exynos5-decon` @0x14830000
     - `dwmmc0/1/2`: @0x13540000/0x13550000/0x13560000
     - `i2c0..N`: HSI2C @0x13870000, 0x13880000, etc.
     - `pmic`: `siliconmitus,sm5703` @I2C 0x49
     - `rtc`: `samsung,s3c2410-rtc` @0x101E0000
     - `watchdog`: `samsung,s3c2410-wdt` @0x101C0000
   - `/chosen`:
     - `dram-base`, `dram-size` (desde DTB S-Boot `memory@`)
     - `boot-args`: `rd=mmcroot0 mmcrw=1 rootrw=1 cpus=4 keepsyms=1 -v`
     - `asmb`: `lp-sip0=0x6f`, `lp-sip1=1` (SIP relajado)
     - `boot-manifest-hash`: SHA-384 fijo
     - `display-scale=2` (540×960 @2x = 270×480 pts)
     - `unique-chip-id`, `chip-id` (sintéticos)
4. **Payload format (`mkpayload_j2.py`):**
   - ARMv7 Mach-O (LC_SEGMENT, LC_UNIXTHREAD)
   - Resolver chained fixups (DYLD_CHAINED_FIXUPS v2/v3)
   - boot-args 608 bytes
5. **Empaquetar BOOT.img (Android boot header v2):**
   - `mkbootimg` con kernel=loader, ramdisk=payload.bin, DTB appended
   - `BOARD_KERNEL_BASE=0x10000000`, `pagesize=2048`, `tags_offset=0x100`
6. **Test local:** Verificar tamaño ≤13 MB, estructura válida.

---

### FASE 2: Drivers Plataforma Críticos (12-20 sem)
**Orden por dependencia (bloquean boot si faltan):**

| # | Driver | Referencia base | Archivos XNU a crear/modificar |
|---|---|---|---|
| 1 | **MCT Timer** | Linux `drivers/clocksource/exynos_mct.c` | `osfmk/arm/arm_mct.c`, `pexpert/arm/J2.h` (MCT base, IRQs), `osfmk/kern/clock.c` hooks |
| 2 | **PMU SMP** | Linux `platsmp.c`, `pmu-exynos3475.c` | `pexpert/arm/pmap.c` hooks, `osfmk/kern/machine_routines.c` `cpu_up/down` |
| 3 | **GIC-400 v2** | XNU `osfmk/arm/arm_gic.c` | Adaptar direcciones, PPI mapping MCT→GIC 102-113 |
| 4 | **DECON Framebuffer** | `ios-galaxy-s8` `loader.c` + Linux `exynos5-decon` | `IOKit/graphics/AppleCLCD.c`, `IOMobileFramebuffer` stub, CPU→FB copy loop |
| 5 | **dwmmc/eMMC Block** | Linux `drivers/mmc/host/dw_mmc.c` + `mmc_core` | `IOKit/IOBlockStorageDriver` subclass, `IOATABlockStorage` compat, HFS+ mount |
| 6 | **I2C Master** | Linux `drivers/i2c/busses/i2c-samsung.c` | `IOKit/IOI2CController` subclass |
| 7 | **Touch zinitix** | Linux `drivers/input/touchscreen/zinitix.c` | `IOHIDFamily` driver, I2C protocol, GPIO interrupt |
| 8 | **PMIC SM5703** | Linux `drivers/mfd/sm5703.c` | Rails LDO/buck para touch, panel, cámara |
| 9 | **Teclas GPIO/sec_touchkey** | Linux `keylayout/sec_touchkey.kl` | `IOHIDFamily` consumer keys (Power, Vol±) |
| 10 | **RTC** | Linux `s3c2410-rtc` | `IOPlatformExpert` `getTimeOfDay`/`setTimeOfDay` |

**Criterio éxito por driver:** Kernel pasa `PE_init_iokit`, driver `probe`/`attach` exitoso, log en framebuffer.

---

### FASE 3: HFS+ Root + Userland iOS 10.3.4 (8-12 sem)

| Paso | Descripción |
|---|---|
| 1 | Descargar IPSW iPhone 5,1 10.3.4 (14G61) 2.0 GB desde `api.ipsw.me` |
| 2 | `mkiosroot_j2.sh`: `apfs-fuse` mount OS.dmg → `hfsprogs` crear HFS+ 4 GB → copiar OS + SystemOS + AppOS cryptexes |
| 3 | `sweet-root-tweaks_j2.py`: skip Setup, seed activation, remove basebandd, disable watchdogs, fix permissions |
| 4 | Flashear HFS+ image a USERDATA (4.8 GB) vía TWRP `dd` o Odin (no fastboot) |
| 5 | AMFI stub (`sweet_amfi.c`) + `unentitled_ios_sim_launch=1` + `launchd_unsecure_cache=1` |
| 6 | IOHID/IOSurface/IOMFB stubs mínimos para backboardd/SpringBoard |
| 7 | Boot args finales: `rd=mmcroot0 mmcrw=1 rootrw=1 cpus=4 keepsyms=1 -v` |

---

## Integración con `j2lte-build` (Rama Local `ios-port`)

### Estructura en repo:
```
j2lte-build/
├── ios-port/
│   ├── xnu/                 # XNU 4570.71.2 source + patches J2
│   ├── shim/                # Loader ARMv7 bare metal
│   ├── tools/
│   │   ├── mkadt_j2.py
│   │   ├── mkpayload_j2.py
│   │   ├── mkiosroot_j2.sh
│   │   └── sweet-root-tweaks_j2.py
│   ├── rootfs/              # HFS+ build scripts (no binarios Apple)
│   └── workflows/
│       └── ios-j2-build.yml # CI macOS + Linux
├── manifests/
│   └── j2lte-ios.xml        # Manifest mínimo (xnu, shim, tools)
└── .github/workflows/
    └── ios-j2-build.yml
```

### CI Workflow (`ios-j2-build.yml`):
```yaml
jobs:
  xnu-armv7:
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
      - name: Build XNU ARMv7 J2
        run: |
          cd ios-port/xnu
          make TARGET_CONFIGS="RELEASE ARM J2" \
            ARCH_CONFIGS=ARM MACHINE_CONFIGS=J2 KERNEL_CONFIGS=RELEASE \
            PLATFORM=iPhoneOS SDKROOT=iphoneos SDKVERSION=10.0 \
            BUILD_LTO=0 BUILD_DYSYM=0 BUILD_WERROR=0 \
            OBJROOT=../build/obj DSTROOT=../build/dst SYMROOT=../build/sym all
      - name: Upload kernel
        uses: actions/upload-artifact@v4
        with:
          name: xnu-kernel-j2
          path: build/obj/**/mach*
  
  shim-armv7:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build Shim ARMv7
        run: |
          cd ios-port/shim
          clang --target=armv7-none-eabi -nostdlib -static -O2 -o loader.elf start.S mmu.c fdt.c console.c
          # Verificar tamaño
      - name: Build ADT + Payload
        run: |
          python3 ../tools/mkadt_j2.py ../build/adt_j2.bin --highmem
          python3 ../tools/mkpayload_j2.py ../build/xnu-kernel ../build/adt_j2.bin ../build/payload.bin --cmdline "rd=mmcroot0 mmcrw=1 rootrw=1 cpus=4 keepsyms=1 -v"
      - name: Package BOOT.img
        run: |
          mkbootimg --kernel ../build/loader.elf --ramdisk ../build/payload.bin --dtb ../dtb_j2.dtb --base 0x10000000 --pagesize 2048 --kernel_offset 0x8000 --ramdisk_offset 0x1000000 --tags_offset 0x100 -o ../build/boot_j2.img
          # Verificar tamaño ≤ 13 MB
  
  hfs-root:
    runs-on: ubuntu-latest
    steps:
      - name: Build HFS+ Root iOS 10.3.4
        run: |
          # Descargar IPSW, apfs-fuse, hfsprogs, mkiosroot_j2.sh
          # Requiere sudo para loop mount → self-hosted runner o workaround
```

---

## Próximos Pasos Inmediatos (HOY)

1. **Crear rama local** `ios-port-fase0` en `j2lte-build`
2. **Setup FASE 0** en workspace local:
   - Clonar XNU 4570.71.2
   - Aplicar patches mt6580 adaptados a J2
   - Crear `MACHINE_CONFIG=J2` inicial
   - Script de build local (simula CI macOS)
3. **Ejecutar build de prueba** → capturar errores de enlace
3. **Documentar resultados** en MD
4. **Esperar tu OK** para crear rama remota y push CI

---

## Preguntas para Confirmar Antes de Empezar

1. ¿Confirmas que **BOOT partition 13 MB** es el target (no recovery)?
2. ¿Tienes acceso a **macOS runner local** o usamos GitHub Actions macOS (gratis para repos públicos)?
3. ¿Quieres que la FASE 0 valide **solo enlace** o también **boot en QEMU** (qemu-system-arm -M virt)?
4. ¿El IPSW iPhone 5,1 10.3.4 lo descargo yo en CI o lo provees tú?

---

*Documento creado para evitar congelamientos. Trabajo incremental por fases.*