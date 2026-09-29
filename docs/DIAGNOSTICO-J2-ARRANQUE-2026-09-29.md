# DIAGNÓSTICO — J2 / LineageOS 20 (A13) — cierre 28/09 noche

Estado completo, con evidencia de logs, para retomar la sesión.

---

## 0. TL;DR

- La animación de arranque **SÍ salía** antes (confirmado por logs, varias veces).
- Los boots buenos son del **27/09 12:43** y **28/09 01:46 → 02:29**.
- El build que se flasheó después (**28/09 18:52**, `eng.runner.20260928.185449`)
  se quedó en el logo. Bloqueo nuevo y distinto: falta
  `libcamera_client_shim.so` (45 `CANNOT LINK` por arranque).
- El cambio de `WITH_DEXPREOPT` (false→true) fue el **28/09 07:05** (`8123120`),
  DESPUÉS de los boots buenos. No explica por sí solo el logo, pero cambia
  el perfil del build (ver §5).

---

## 1. Evidencia dura: la animación SÍ funcionaba

Extraído del historial opencode (sesión `ses_fbfb12c19ffefIkzqa7FR5obRD`),
snapshots `st_prop_*.txt` y volcados de `getprop init.svc.*`:

```
2026-09-27 03:42 | bootanim: running  | surfaceflinger: running | zygote: restarting
2026-09-27 03:43 | bootanim: running
2026-09-27 04:05 | bootanim: running  | zygote: restarting
2026-09-27 12:43 | bootanim: running  | surfaceflinger: running | zygote: restarting  (st_prop_3)
2026-09-28 01:46 | zygote: running | audioserver: running | netd: stopped
                 | surfaceflinger: running | bootanim: running
2026-09-28 02:05 | bootanim: running | zygote: running
2026-09-28 02:14 | bootanim: running | zygote: running
2026-09-28 02:28 | bootanim: running | zygote: running            (st_prop_5)
2026-09-28 02:29 | zygote: running | audioserver: running | netd: stopped
                 | surfaceflinger: running | bootanim: running
```

**El mejor estado fue 28/09 01:46–02:29:** `zygote=running`,
`audioserver=running`, `surfaceflinger=running`, `netd=stopped` (el override
del bucle), `bootanim=running`. Todo a la vez. Eso es "llegaba tan lejos".

Dato clave: en los boots del 27/09 la animación salía **aun con
`zygote=restarting`**. O sea, el "bloqueo que nunca terminaba la animación"
era el loop de zygote/netd, no la animación en sí. La animación arrancaba
igual (surfaceflinger ya corría) pero el sistema no completaba el arranque.

---

## 2. La cadena de bloqueos (por qué tapaba lo otro)

Cada fix destapaba el siguiente. Por eso el shim de cámara pasó desapercibido
durante días:

| # | Bloqueo | Síntoma | Fix | Estado |
|---|---------|---------|-----|--------|
| 1 | netd mataba a zygote (loop ~5 s) | zygote `restarting` | `4d96222` (override onrestart) | ✅ en build |
| 2 | composer sin transporte `hwbinder` | SF aborta, sin HW | `d0709f4` | ✅ en build |
| 3 | HAL de audio AIDL crasheaba (SIGABRT) | tombstones, `audioserver` restart | FIX-034b/c/f/g (deshabilitado) | ⛔ on-device |
| 4 | **falta `libcamera_client_shim.so`** | **45 CANNOT LINK** | FIX-036 `c0327a0` | 🔧 pusheado, falta rebuild |

El #4 es el actual. Tumba `app_process` (→ zygote nunca levanta) y
`bootanimation` (→ sin animación). Por eso el logo.

---

## 3. Causa raíz del estado actual (FIX-036)

Del log del último boot (`/data/local/tmp/bootlog.txt`, 16:31,
build `eng.runner.20260928.185449`):

```
F linker : CANNOT LINK EXECUTABLE "/system/bin/bootanimation":
  library "/vendor/lib/libcamera_client_shim.so" not found:
  needed by /system/lib/libcamera_client.so in namespace (default)
```

45 ocurrencias. Binarios caídos: `app_process`, `bootanimation`,
`audioserver`, `mediaserver`, `cameraserver`, `mediaextractor`.

El shim **no existe** en la imagen (`find` → 0). FIX-002 (22/08) lo sacó de
`PRODUCT_PACKAGES` con el supuesto falso "no tiene fuente ni blob". **El
supuesto era incorrecto**: `libshims/libcamera_client/CameraParameters.{cpp,h}`
existe y compila limpio (solo `const char[]`, sin deps).

El shim exporta las constantes `CameraParameters` que el blob de cámara de
Samsung necesita y que `libcamera_client` de AOSP **no** define
(`PHASE_AF`, `RT_HDR`, `METERING_CENTER`, `DYNAMIC_RANGE_CONTROL`,
`PIXEL_FORMAT_YUV420SP_NV21`, `EFFECT_CARTOONIZE`, … — 0 ocurrencias en la
lib de AOSP).

**Fix (pusheado):** `c0327a0` en
`android_device_samsung_universal3475-common` / `lineage-17.1` — vuelve a
declarar `libcamera_client_shim` en `PRODUCT_PACKAGES`. El manifest
(`manifests/j2lte.xml`) ya consume esa rama → la build lo toma sin tocar
nada más. Validado en local: compila y exporta los símbolos correctos.

---

## 4. El "sí salía la animación" vs el logo de hoy — línea de tiempo

| Fecha/hora | Qué pasó |
|---|---|
| 22/08 | FIX-002 excluye `libcamera_client_shim` de la build (error latente) |
| 11/09 20:55 | `a867ad2` pone `WITH_DEXPREOPT=false` |
| 27/09 03:42–04:05 | animación corre (con zygote aún restarting) |
| 27/09 10:30 | `5289e63` arreglo gpsd/ADB/cámara |
| 27/09 12:43 | animación corre (st_prop_3) |
| 27/09 22:35 | `4d96222` fix del loop netd→zygote |
| **28/09 01:46–02:29** | **mejor estado: todo running** (st_prop_5) |
| 28/09 07:05 | `8123120` `WITH_DEXPREOPT=false`→`true` |
| 28/09 18:52 | build `eng.runner.20260928.185449` flasheada → **logo** |

---

## 5. Sobre el cambio de dexpreopt y las "103 apps"

El "se ponía a optimizar 103 apps de fondo" corresponde al build con
`WITH_DEXPREOPT=false` (el de los boots buenos): al no preoptimizar el dex
en la build, el teléfono corría `dexopt` de ~103 apps en el primer arranque.
**No era un bug**, era el comportamiento de ese build.

`8123120` (28/09 07:05) lo pasó a `true` (workflow línea ~2301) para evitar
ese primer-boot dexopt y tener apps AOT con JIT off. Efectos:
- Menos trabajo en el primer boot (bueno), pero
- build más pesado/lento de compilar (contexto: storage/ENOSPC), y
- perfil de arranque distinto al de los boots buenos.

**No es la causa del logo** — el logo es el shim ausente (#4), que es
independiente. Pero el build de hoy NO es el mismo build que llegaba lejos:
si querés reproducir exactamente el punto "bueno", conviene volver a
`WITH_DEXPREOPT=false` al menos para el siguiente ciclo de diagnóstico.

---

## 6. Contradiccia pendiente (honesta, sin resolver)

El `libcamera_client.so` **en disco** (mtime 2009, nunca modificado por mí)
**no tiene** el `DT_NEEDED` del shim — verificado 2× con `readelf`/`objdump`
sobre copia extraída. Pero el **runtime** sí lo exige como
`needed by /system/lib/libcamera_client.so`.

No pude determinar quién inyecta ese NEEDED. Hipótesis principal: el
**linkerconfig generado en runtime** (tabla `TARGET_LD_SHIM_LIBS`
`original|shim`). No confirmada.

**Si el fix del shim no basta, esto es lo primero a investigar**: puede que
haya que quitar el NEEDED en vez de (o además de) satisfacerlo.

---

## 7. Qué sigue (en orden)

1. **Lanzar la build** con el fix (vos, GitHub Actions). El manifest ya
   toma `lineage-17.1` → toma el shim solo. Único paso bloqueante.
2. **Flashear + reiniciar.** Verificar, en orden:
   - `grep -c 'CANNOT LINK'` en el log → debe bajar a 0.
   - `getprop init.svc.zygote` → `running` (no `restarting`).
   - `getprop init.svc.bootanim` → `running`.
3. **Si aparece la animación pero no termina:** el siguiente bloqueo está
   más arriba. Diagnosticarlo con el mismo método (leer el `CANNOT LINK` /
   el crash del log). Objetivo de referencia: el estado 28/09 01:46.
4. **Decisión abierta:** si el shim no alcanza, ¿el plan B es quitar el
   NEEDED (investigar el mecanismo de runtime) en vez de seguir buscando
   shims? (Quedó planteado; sin respuesta aún.)

Para volver exactamente al punto "llegaba lejos": considerar
`WITH_DEXPREOPT=false` en el próximo ciclo de diagnóstico.

---

## 8. Commits de esta sesión

- `android_device_samsung_universal3475-common` / `lineage-17.1`:
  - `c0327a0` — FIX-036, empaqueta `libcamera_client_shim` (el fix real).
- `j2lte-build` / `feat/selfhosted-runner`:
  - `0ab0b31` — doc `docs/fixes/FIX-0036-camera-client-shim.md`.

Commits previos relevantes (ya en el árbol):
`4d96222` (netd→zygote), `d0709f4` (composer hwbinder),
`8123120` (dexpreopt true), `a867ad2` (dexpreopt false), `5289e63` (gpsd/ADB/cámara).

---

## 9. Notas operativas (para no tropezar mañana)

- ADB: `"/mnt/c/Program Files/Software Fix/adb.exe"` (única ruta que funciona).
- adb queda `offline` durante el boot de sistema; entra a TWRP para editar.
- `/system_root` requiere `mount -o rw /dev/block/mmcblk0p20 /system_root`
  en cada ingreso a TWRP.
- `adb exec-out cat <archivo>` es la forma confiable de extraer binarios
  (`adb shell` los corrompe).
- El usuario flashea / lanza builds / reinicia; el asistente no.
- Evidencia persistente en el teléfono: `/data/local/tmp/{bootlog.txt,
  st_prop_*.txt, st_crash_*.txt, st_dmesg_*.txt, st_ps_*.txt, st_mem_*.txt}`.
- Historial opencode consultable vía SQLite:
  `~/.local/share/opencode/opencode.db` (tablas `part`/`message`; el output de
  los comandos está en `part.data` como JSON, `state.output`). `sqlite3` no
  está instalado → usar `python3 sqlite3` (copiar la db primero por locking).
