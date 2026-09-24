# BOOT_HANG_INVESTIGATION — J2LTE LOS20, logo Samsung estático

## Estado conocido
- LOS20 compila (systemimage/bootimage/recoveryimage/bacon/OTA OK). Instalación limpia OK, TWRP intacto.
- Síntoma: logo Samsung estático ~5 min, sin animación, sin USB/ADB, sin reboot.
- Sin traza kernel (pstore/last_kmsg vacíos, INFORM3 ilegible, snapshot pisado por TWRP, sin UART).
- Boot.img válido e idéntico a stock en estructura. DTB `_00` primero (revertido; `_04` probado sin cambio).
- fbcon compilado pero sin texto: hang anterior al registro de fb0/DECON.
- Referencia que bootea: `~/reference_universal3475_l19` (`lineage-19.1_ext4-backport`, `3192493d`, kernel 3.10.108) + device LOS19 (`~/reference_j2lte_l19`, `~/reference_universal3475-common_l19`).

## Hipótesis DESCARTADAS (no re-investigar sin evidencia nueva)
1. **DTB order** — `_04` primero probado físicamente, mismo síntoma. Revertido a `_00→_04` (= ref). FIX-0001 DESCARTADO.
2. **FRAMEBUFFER_CONSOLE** — compilado (`fbcon.o` en binario CI), binding automático, consola correcta; pantalla intacta ⇒ hang pre-DECON, no defecto fbcon. FIX-0002 queda como instrumentación, no como causa.
3. **ION heaps** (`common` 6M + `video` 48M) — path fb usa system heap con errores no-bloqueantes; reserva sin loops; ref bootea sin ellos pero nada puede colgarse ahí.
4. **HSI2C/SM5703/MUIC** — código/DT/IRQs idénticos a ref; único loop sin cota existe en ambos; MFD no-universal ni compila.
5. **TRUSTONIC_TRUSTED_UI** — solo activable por ioctl userspace; sin enganche boot; símbolo TOUCH muerto.
6. **ueventd/by-name, /system no montado, tags_addr, boot.img, EXT4-backport-como-logo-causa, ZRAM-como-logo-causa** — refutados con código/fuente.

## Candidatos pendientes
- ~~Binder 64-bit~~ → ver veredicto abajo (descartado como causa, pendiente como compatibilidad).

## Veredicto Binder (ronda dedicada)
- **DESCARTADO COMO CAUSA DEL HANG ACTUAL.**
- Evidencia: version-check ocurre en `open()` userspace (servicemanager `on init`); mismatch ⇒ abort ⇒ `critical` ×5 ⇒ `LOG(FATAL)` ⇒ reboot a bootloader (`service.cpp`, `reboot_utils.cpp`); sin supresión `no_fatal` en el árbol; servicemanager es `critical` (rc:5). El síntoma predicho son ciclos de reboot, incompatible con logo estático 5 min sin reboot ni modo download.
- USB/adbd arrancan DESPUÉS de servicemanager (triggers `on boot`/propiedades): un fallo binder tampoco explicaría la ausencia de USB sin reboot previo.
- **CANDIDATO DE COMPATIBILIDAD POST-BOOT**: con binder-32, T nunca levantará servicios; obligatorio antes de animación, después de resolver el hang.
- Punto temporal real de fallo binder: apertura de `/dev/binder` por servicemanager (second stage, post-USB-init-path pero pre-adbd-enumeración).

## Estado: DIAGNÓSTICO ESTÁTICO AGOTADO
Fase estática pre-DECON sin causa demostrable del hang (todo idéntico a ref que bootea salvo tunings sin mecanismo). Primera evidencia necesaria para continuar: traza del kernel (UART jig: único método sin reboot-dependencia ni persistencia).

## Registro de investigación
### Ronda 1 — Timer/clocksource MCT: DESCARTADA
Driver `exynos_mct.c` idéntico (`diff -q`); sin loops (`while/poll` ausentes); HZ/RCU solo tuning. Sin mecanismo de hang.
### Ronda 2 — MobiCore/fastcall: SIN DIFERENCIA FUNCIONAL
Único diff `fastcall.h`: guard `__clang__` para `MC_ARCH_EXTENSION_SEC` (equivalencia compilación, mismo runtime que GCC≥4.5.2). Sin mecanismo.
### Ronda 3 — Descompresor/head.S: SOLO SINTAXIS CLANG
`Makefile`, `head.S`, `piggy.gzip.S` difieren en sintaxis asm (`"ax"` vs `#alloc,#execinstr`, `cc-option` guards) con semántica idéntica. Confirma toolchain Clang vs GCC de ref: bucket no verificable estáticamente.
### Candidatos pendientes
- Binder 64-bit: mecanismo DEMOSTRADO (version-check + abort + Kconfig "break newer user-space"), fatal para servicios T; pero mismatch daría reboot-cycles vía critical, no logo estático → no explica síntoma actual, obligatorio después.
- UART/pstore: sin vía sin hardware/build nueva.

## Estado: PARADA PARA AUTORIZACIÓN
Fase estática pre-DECON agotada sin causa demostrable del hang (todo idéntico a ref que bootea salvo tunings). Única acción con mecanismo demostrado pendiente: binder-64 (requiere autorización por ser cambio de código).

## Ronda PC+J2+USB-sin-hardware: AGOTADA
Canales auditados para evidencia persistente del kernel sin UART ni build nueva:
- pstore/ramoops: sin región segura (TWRP pisa toda DRAM no reservada); params exigen reserva inexistente. NO VIABLE.
- sec_debug/snapshot/last_kmsg/INFORM3: ilegible (MMIO) o sobrescrito (RAM). NO VIABLE.
- eMMC (CACHE/MISC/PARAM): sin backend en 3.10 (`pstore-blk` no existe); PARAM sin driver. NO VIABLE.
- fbcon: cubre post-DECON únicamente; hang es anterior. Ya probado.
- framebuffer-bootloader como lienzo: sin escritor pre-DECON en el árbol. NO VIABLE sin código nuevo.
- Watchdog como conversor hang→reset: posible pero sin texto; armarlo exige cambio y rompe boots lentos buenos (dexopt 10+ min con `watchdogd` disabled). NO RECOMENDADO.
- `watchdogd` existe (`init.universal3475.rc:787`) pero disabled (requiere `ro.debug_level=0x4f4c`).
- Download/Odin: solo flash, sin dump RAM. MTP/TWRP: sin kernel LOS20.
CONCLUSIÓN DE RONDA: con PC+J2+USB y sin modificar nada, la evidencia del kernel es INALCANZABLE por diseño. Próximo paso real: (1) jig UART (sin build), o (2) FIX autorizado con mecanismo demostrado (ninguno pendiente con evidencia salvo binder post-síntoma).

## Ronda — Investigación externa/histórica (web + comunidad)
Fuentes revisadas: XDA (J2-17.1 cyanogen, J1-18.1/19.1 SluckWare: binder32→binder64 con boot siempre pasando logo), Codeberg kOtusin (3.10 arranca Android 12 sin reescribir kernel), 7420_patches LOS20 (fallos tardíos userspace, no logo), S21-Ultra LLVM=1 (**caso espejo**: logo estático + sin adb + TWRP OK + sensible a toolchain), Magisk boot.img (fallo pre-USB por empaquetado), ClangBuiltLinux (3.10+Clang sin LLVM=1 = binario que compila pero no arranca), MCT/DECON/Trustonic/binder priors.
Conclusión: lo más parecido al caso (logo+sin USB+sin reboot+TWRP OK+sensible a kernel) es **toolchain/imagen (H10)**, DT j2 (H13/H14) y empaquetado (H11); binder/userspace (H16/H7) fallan TARDE (animación/adb/reboot), no encajan.
## Ronda — Comparación kernels históricos (ref: 17.1 vs 19.1_ext4-backport)
NO existe rama lineage-18.1. Delta 17.1→19.1 = 410 commits: ~403 ext4/jbd2 + 7 deps (vfs, buffer, KernelSU-32628). **defconfig j2 idéntico** (md5), **DT j2 idéntico**, cero cambios boot/display/toolchain/binder/ion/mali/SMP/timer. El backport (mount-checks estrictos, journal-checksum, reservation API, `panic` en abort) endurece montaje, no explica logo estático. Binder-64 y ZRAM+LZ4 viven en la línea 17.1, previos al fork.
## Ronda — Binario/boot (artefactos existentes, solo pipes)
Stock: sin DTB anexado válido, gzip ramdisk 49 entradas (init Samsung), `SYSMAGIC000K`, second en 0x10f00000. Build 1: header idéntico salvo `name` vacío/second ausente/cmdline; kernel +20.6% con **5 DTB válidos verificados** (totalsize/ver 17); ramdisk XZ 13 entradas (first-stage Lineage); **decompressor stub distinto sin banner "Uncompressing Linux"** (toolchain distinta); sin IKCONFIG/Linux-version en crudo (comprimido).
## Ronda — Toolchain
Nuestro: Clang r450784d (`BoardConfigCommon.mk:72-74`) + sintaxis asm adaptada (Makefile/head.S/piggy). Ref: GCC (sintaxis GAS `#alloc`). Caso espejo S21 demuestra que en Exynos la toolchain decide boot/no-boot con síntoma idéntico. No verificable estáticamente: requiere build A/B.
## Ronda — Bootloader
SBOOT J200M sin docs públicas; logo = bootloader (NO demuestra ejecución Linux: el kernel podría no haber arrancado jamás). Download/Odin sin dump. Param sin driver.
## Ronda — Watchdog como discriminador sin UART: CERRADO (sin valor)
- Bloque `watchdogd` idéntico a la ref: `service watchdogd /sbin/watchdogd 10 20` (init.universal3475.rc:787), `on property:ro.debug_level=0x4f4c` + `start watchdogd` (:797-798) — mismo que ref (:802/811-813).
- `ro.debug_level` NO lo setea nadie en el árbol (solo la condición de arranque) → watchdogd jamás patea el WDT en líos.
- WDT arrancado por el driver S3C2410 solo si `tmr_atboot`; el logo estático de 5 min sin reset es CONSISTENTE: WDT apagado por diseño → sin fuente de reset → el hang no se convierte en reboot. Sin build no hay cómo encenderlo como discriminador (sería cambio funcional → requiere FIX autorizado). CERRADO sin evidencia nueva.
## Ronda — Casos comunitarios
Tabla en informe extendido: J1-18.1/19.1 (bootea, binder64), 7420-LOS20 (parches userspace tardíos), S21-LLVM1 (espejo exacto), Magisk-pack (pre-USB), MCT/DECON/Trustonic priors, binder32/64 (tardío).
## Ronda — Toolchain (experimento A/B Clang vs GCC): EN CURSO (vía CI)
- Hypothesis H10 (toolchain) se testea con A/B controlado: A=clang r450784d (actual), B=GCC 4.9 prebuilt (`TARGET_KERNEL_CLANG_COMPILE=false`). Mismo source (e5f60f70), defconfig, DTB order, ramdisk, header.
- Bloqueo local: clang-14 no ejecuta en WSL1 (`Exec format error`, kernel 4.4.0-17763) → A/B se corre en CI (workflows stages: input `kernel_toolchain`, upload kernel forense). Ver /tmp/opencode/ab/.
- Criterio: B arranca (animación/adb) → H10.confirmada, GCC pasa a ser la toolchain del árbol. B también logo estático → H10.descartada y volvemos a H13/H14 (DT/tempranas).
- ⚠️ Primer intento A/B (runs 36035823901 clang / 36035786966 gcc, SHA 9b1ac55): ambas fallaron en `stage-build` con **Error 127 (command not found)**, NUNCA llegaron a compilar el kernel. FORENSE (comprobado en logs + código): la variable de entorno `KERNEL_TOOLCHAIN` (job env del experimento) COLISIONA con la variable interna de make `KERNEL_TOOLCHAIN` de `vendor/lineage/config/BoardConfigKernel.mk:129` (`?=`). Al venir del entorno, make NO la sobreescribe: `CROSS_COMPILE="clang/arm-linux-androidkernel-"` (A) y `" gcc/arm-linux-androidkernel-"` (B) → `/bin/sh: 1: clang/arm-linux-androidkernel-ld: not found`. Todo lo demás funcionó: input propagado, rama GCC correcta (CFLAGS_MODULE=-fno-pic, sin CC/CLANG_TRIPLE), inyección efímera de `TARGET_KERNEL_CLANG_COMPILE := false` aplicada antes de lunch, regla reset limpia. **H10 NO testeada** (nunca hubo compilación) → no confirmable/descartable todavía.
- FIX mínimo (local, SIN commit): renombrar env a `AB_KERNEL_TC` en ambos workflows (stages + full), path de `ld.lld` vía `$GITHUB_PATH` (canónico), y upload forense copiando a `$GITHUB_WORKSPACE/kernel-forensic/` (upload-artifact@v4 no acepta rutas absolutas fuera del workspace). Pendiente: autorización de commit + rerun del par A/B.
