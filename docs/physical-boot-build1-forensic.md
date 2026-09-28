# Physical Boot Build 1 — Forensic Investigation

## 1. Estado actual
ROM LOS20 Build 1 (`lineage-20.0-20260922-UNOFFICIAL-j2lte.zip`, bacon `35755859185`) instalada limpia desde TWRP sin errores. Primer boot: ~5 min en logo Samsung, sin animación Lineage, sin USB/adb. Vuelta a TWRP: `last_kmsg`/`pstore` vacíos, TWRP intacto. Particiones: BOOT mmcblk0p10, RECOVERY p11, SYSTEM p20 (1249 MB nuestros), USERDATA p23, EFS p3, CPEFS p4.

## 2. Evidencia confirmada
- Instalación limpia OK; TWRP sobrevive; SYSTEM monta en TWRP con nuestro contenido.
- `boot.img` estructuralmente válido: magic `ANDROID!`, addrs = stock (`0x10008000`/`0x11000000`), page 2048, `tags_addr 0x10000100` = stock, FDT en kernel+`0x90c` idéntico a stock, `SEANDROIDENFORCE` presente.
- `dmesg.log` aportado = kernel TWRP (proceso `recovery:2110`), NO nuestro boot. `recovery.log` = sesión TWRP (SAR-DETECT SAR, particiones sanas).
- Sin panic/oops/pstore/last_kmsg recuperables.

## 3. Evidencia que NO tenemos
Ni una línea del kernel LOS20: dmesg es de TWRP; pstore/last_kmsg vacíos por diseño (sin `PSTORE_CONSOLE`, sin región ramoops); INFORM3 ilegible (MMIO bloqueado); snapshot/sec_debug sobrescritos por TWRP.

## 4. Datos obtenidos desde TWRP
Particiones y tamaños (ver §1); `ro.revision=4`, bootloader `J200MUBU2ARB1`; `/dev/mem` existe (RAM legible, MMIO no); sec_debug magic `0x276f45f9` (basura TWRP); snapshot sin banner/panic/marcadores.

## 5. Evidencia del boot.img
Ver §2. Entrega bootloader→kernel exonerada.

## 6. Investigación kernel/DTB
DTB appended x5, kernel toma el primero (`head.S`); orden actual `_04` primero (rev 4 ∈ `[4,255]`, catch-all correcto). Display/panel/DECON idénticos en las 5 variantes; difieren regulador cámara, touchscreen, GPIOs, audio. `CONFIG_RD_XZ=y`. Clang FIX-029 + AEABI FIX-031b (primer boot Clang histórico).

## 7. Investigación init/ramdisk
First-stage AOSP 13; `mount_all /fstab`, imports relativos; `/system` se monta vía `TrySwitchSystemAsRoot` (ignora `recoveryonly`); `ueventd.universal3475.rc` cargado por ruta legacy; `by-name` genérico. `ro.hardware` provisto por bootloader (TWRP: universal3475).

## 8. Investigación display/GPU
`EXYNOS_DECON_EXYNOS3475=y`, panel `s6e88a0` común; Mali blobs + shims declarados. Sin evidencia de fallo (logo persiste = framebuffer nunca reprogramado).

## 9. Investigación storage/mounts/SELinux
SYSTEM/CACHE/USERDATA montables (TWRP); fs_config solo `[cpefs/]` + placeholder + micro-spec `system_file` (verificado en CI); resto specs plat.

## 10. Mecanismos de logging disponibles
sec_debug/INFORM3/snapshot (existentes pero sobrescritos o ilegibles post-TWRP); pstore/ramoops (requiere build + dirección sin demostrar); UART jig 619K (mejor opción, sin cambios, requiere hardware); `DEBUG_LL`/earlyprintk (requieren build).

## 11. Pruebas de solo lectura posibles
`adb devices`; `adb shell` lecturas (`/dev/mem` RAM, `/proc`, `/sys`, `md5sum` del ZIP en teléfono); `ls /sys/fs/pstore`, `cat /proc/last_kmsg` (vacío esperado). Comandos exactos documentados en el informe del 2026-09-23.

## 12. Pruebas que requieren autorización
Cualquier reboot/flash/build; jig UART (hardware externo); `PSTORE_CONSOLE`/ramoops (build nueva); extracción de `recovery.img` para flasheo.

## 13. Hipótesis (hechos vs inferencias)
- H1 hang/panic kernel pre-display (inferencia principal): logo persistente + sin USB + sin reboots.
- H2 fallo init/mounts retenido por sec_debug sin reboot (inferencia alternativa).
- Hecho: sin traza kernel, ninguna es confirmable. Binder-32 y DTB-orden son fixes estructurales pendientes de prueba, no causas demostradas del logo.

## 14. Próximo paso recomendado
Jig UART 619K (máxima evidencia, cero cambios) o nuevo boot instrumentado con `adb` vigilado; en paralelo, tanda de fixes estructurales (binder-64, DTB-04 ya aplicado) cuando se autorice build.
