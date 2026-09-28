# Bacon — Run 35755859185

## 1. Resumen ejecutivo

Run `LOS20 build full (guarded)` en estado **success**: `mka --skip-soong-tests bacon` completó 93366/93366 objetos (05:22:55) y produjo la ROM `lineage-20.0-20260922-UNOFFICIAL-j2lte.zip` (565 MB) vía `target_files` + OTA completa. Cero `FAILED`, cero errores fatales, cero `neverallow`. Conteo de hallazgos: 🔴 0, 🟠 0, 🟡 3 (todos con explicación benigna o acotada), resto 🟢. Nada requiere corrección antes de usar el resultado como base de prueba (el flasheo en sí sigue pendiente de autorización separada).

## 2. Datos de la run

- Run: https://github.com/MATYJAGUZ075/j2lte-build/actions/runs/35755859185
- Workflow: `LOS20 build full (guarded)`, job `build`, modo `full-bacon`.
- Infra: `46e13c8` (filtro Option A con mkdir/dedup/path-absoluto en `full.yml`); device con placeholder (`68aced3`) + micro-spec `/cpefs` (`e083077`).
- Duración: 5h38m (16:42→22:20 UTC 2026-09-22).
- Artefacto `j2lte-target-files-35755859185` (1.1 GB): `bacon.log`, `disk-monitor.log.gz`, telemetría y **2 zips de idéntico md5** (`47a1058b…`): `lineage-20.0-20260922-UNOFFICIAL-j2lte.zip` + `lineage_j2lte-ota-eng.runner.zip` (el step sube ambos nombres; es un único OTA full, sin payload A/B — esperado en dispositivo A-only).

## 3. Resultado de mka bacon

- `[100% 93366/93366] build bacon` → `Package Complete: out/target/product/j2lte/lineage-20.0-20260922-UNOFFICIAL-j2lte.zip` → `#### build completed successfully ####`.
- Kernel: `Building Kernel Image (zImage-dtb)` + `arch/arm/boot/zImage-dtb is ready`; `boot.img` (7.4 MB) dentro del zip.
- Contenido del zip (12 archivos): `system.new.dat.br` (545 MB), `boot.img`, `update-binary`, `updater-script`, `otacert`, `metadata`/`metadata.pb`, `system.transfer.list`. Sin `payload.bin` (correcto: no A/B).

## 4. Target files y empaquetado

- `target_files-eng.runner.zip` intermedio OK; `add_img_to_target_files`: `system.img`, `userdata.img` (1179648 bloques), `cache.img` (49368 bloques) creados sin errores.
- `ota_from_target_files` (22:18:24→22:19:45): `done`, OTA full con test-keys eng (esperado en `userdebug` sin firma release).
- `data/`: `ls: out/target/product/j2lte/data: No such file or directory` (línea 235640) — benigno: chequeo `if [ -d … ]` del empaquetado; no hay partición DATA separada que empaquetar. 🟢 (cierra el pendiente del análisis systemimage).

## 5. Análisis de imágenes regeneradas

- `system.img` regenerada dentro de `target_files` vía `mkuserimg_mke2fs/e2fsdroid` sin errores (el wall `/cpefs` de runs anteriores no reapareció).
- `userdata.img`/`cache.img` generadas (solo existen dentro del target_files/OTA; no se suben como artefactos sueltos — esperado).
- `boot.img`/`recovery.img` del 99% reutilizadas (`using prebuilt … from BOOTABLE_IMAGES`).

## 6. OTA / payload / metadata

- OTA full (no incremental: primer build, sin base previa). `metadata` + `metadata.pb` + `otacert` presentes.
- `brillo_update_payload` compilado como host tool pero sin `payload.bin` en el zip: correcto para A-only (sin motor A/B).
- Claves: test-keys de ingeniería (`eng.runner`); para release haría falta firma propia (fuera de alcance, anotado, no bloqueante).

## 7. SELinux / file_contexts / e2fsdroid / fs_config

- `/cpefs`: cero líneas `searching for label`; placeholder instalado al 1%; micro-spec `system_file` funcionó también en bacon. ✅ (cierra runs #33/#34).
- `checkfc`/`sefcontext`: cero errores (el wall `bluetooth_device` de las runs 72% quedó atrás con el micro-archivo).
- `neverallow`: 0. `file_contexts.bin` estándar (local+modules+device).
- `system/vendor/*` bajo genérico plat `vendor_file` (sin errores de etiquetado).
- `ignored token "selabel=…"` de e2fsdroid: informativo (formato capabilities no usado). 🟢

## 8. Almacenamiento y Option A

- Filtro (job log): `XARGS_EXIT=0`, `filtrados OK: 1077 FAIL: 0`, `.repo` 21G→1.3G, `FREE 61181→80427 MB`, **`RECUPERADO_MB=19246`**. G1–G5 todos OK.
- Disco final: 145G, 138G usados, **6.7G libres (96%)**, inodos 11%. Ajustado pero suficiente (el build ya terminó; margen menor que en systemimage por el árbol OTA extra).
- `system.img` staging 1244 MB < 2048 MB. Sin ENOSPC en ningún punto.

## 9. Warnings y errores encontrados

| Severidad | Mensaje | Ubicación | Impacto | Evidencia | ¿Acción? | Sugerida |
|---|---|---|---|---|---|---|
| 🟢 | 19019 `warning:` (7588 unused-args, 7001 unused-variable, resto toolchain/Java) | todo el log | ruido Clang/JDK sobre AOSP13 | 0 `FAILED`, build verde | no | ninguna |
| 🟢 | `overriding/ignoring old commands` (libhwjpeg, mali, drmclearkeyplugin) | Makefile/installs-*.mk | duplicados conocidos resueltos | preexistente | no | ninguna |
| 🟢 | `defconfig: warning: override: BROADCOM_WIFI` | Kconfig j2lte | override conocido del defconfig | preexistente | no | ninguna |
| 🟢 | `warn: removing resource …send_to_voicemail_import_failed without required default value` (aapt, dialer) | ~85% | overlay sin default; warning de recursos | build verde | no | ninguna |
| 🟢 | `ls: …/data: No such file` | empaquetado | chequeo guardado, sin DATA separada | esperado | no | ninguna |
| 🟡 | zips ota/bacon con md5 idéntico | artifact | un único OTA full (sin incremental/payload); esperado en primer build A-only, pero confirma que no hay variante diferencial | md5 iguales | no ahora | al iterar, evaluar OTA incremental |
| 🟡 | 6.7G libres al final (96%) | telemetría | margen justo; futuros builds con más contenido podrían acercarse al límite | 8.4–13G en runs previas | vigilar | ninguna ahora |
| 🟡 | firma test-keys eng | OTA/metadata | esperado en userdebug; release requerirá claves propias | nombre `eng.runner` | no ahora | planificar firma release antes de distribuir |
| — | `FAILED`/`error:`/`denied`/`undefined reference`/`No space left` reales | — | **cero** | grep exhaustivo | — | — |

## 10. Problemas potencialmente peligrosos

Ninguno con evidencia. El único candidato histórico (`/cpefs`) está verificado ausente en este log (0 líneas). `system/vendor/*`, fs_config mergeado (4236 entradas), userdata/cache, boot/recovery: todos verdes.

## 11. Cosas normales/esperadas

Warnings `-Wunused-*`, `Copy:` de staging, edges `.meta_lic`, `Generate file_contexts` de APEX, `rm -rf/mkdir` de dumps AIDL, `aidl_hash_gen`, `fs_config` host, `ignored tokens`, ` vibrational `… (ruido estándar AOSP). `brillo_update_payload` compilado sin uso (A-only). `tool-cache`/`preinstalled-runtimes` y Node24: avisos de Actions, ver §12.

## 12. Infraestructura GitHub Actions

- `Node 20 is being deprecated… running with Node 24` + `tool-cache renamed to preinstalled-runtimes` (job log inicio): avisos de plataforma, cero relación con el build Android. Separable y futuro: renombrar input cuando sea conveniente (no urgente; funciona hasta v3.0.0).
- Steps infra (free-disk-space, Swift, off-out, deps, guard) todos success; rescate/telemetría sin uso (no hubo fallo).

## 13. Archivos/código relacionados

- Filtro/guards: `los20-build-full.yml` (XARGS_EXIT=0 en job log).
- Spec: `device/.../sepolicy/vendor/file_contexts_cpefs` + `LOCAL_FILE_CONTEXTS` en `libsecnativefeature/Android.mk`.
- Placeholder: `configs/cpefs.placeholder` + `device-common.mk`.
- Salida: `out/target/product/j2lte/lineage-20.0-20260922-UNOFFICIAL-j2lte.zip` (artifact).

## 14. Acciones recomendadas

1. Ninguna bloqueante. (Todo lo hallado es 🟢/🟡 explicado.)
2. A futuro: firma release (claves propias) antes de cualquier distribución; evaluar OTA incremental en siguientes builds; vigilar margen de disco (~7G) si crece el contenido.
3. No reabrir: `/cpefs`, `checkfc`, Option A, ENOSPC — verificados verdes aquí.

## 15. Conclusión

- Confirmado: bacon 100% verde con ROM + OTA full generados; `/cpefs` ausente del log de errores; `data/` benigno y cerrado; Option A con ~18.8 GB recuperados y guards OK; disco final 6.7G sin ENOSPC.
- Pendiente de autorización separada (no técnica): flasheo/prueba de la ROM.
- Bloqueos conocidos antes del siguiente paso: ninguno.
