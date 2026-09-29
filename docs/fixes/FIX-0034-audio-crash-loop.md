# FIX-0034 — Crash-loop de `android.hardware.audio.service` (bloquea boot)

## Problema
LOS20 A13 en J2 queda sin animación. El service que se crashea en loop es
`/vendor/bin/hw/android.hardware.audio.service`, con

```
Abort message: 'Binder threadpool cannot be shrunk after starting'
  android::ProcessState::setThreadPoolMaxThreadCount(unsigned int)
  ABinderProcess_setThreadPoolMaxThreadCount
  main+90  /vendor/bin/hw/android.hardware.audio.service
```

Tombstones: docenas seguidas de `android.hardware.audio.service` con pids
crecientes (~11-16 tombstones por boot de 2 min antes del fix).

## Causa raíz (real, requiere rebuild)
Bug de ORDEN en `hardware/interfaces/audio/common/all-versions/default/service/service.cpp`:

```
ProcessState::initWithDriver("/dev/vndbinder");
ProcessState::self()->startThreadPool();       // marca mThreadPoolStarted
ABinderProcess_setThreadPoolMaxThreadCount(1); // LOG_ALWAYS_FATAL si ya started
```

`startThreadPool()` activa internamente el pool, y llamar a
`setThreadPoolMaxThreadCount(1)` después aborta ("cannot be shrunk after
starting").

## Fix de raíz — GITHUB (patch aplicado por el workflow en cada build)
Repo `MATYJAGUZ075/j2lte-build`, branch `main` (commit `80a97bf`) y
`feat/selfhosted-runner` (commit `1af8c27`).
Archivo: `patches/hardware-interfaces/0001-audio-service-set-threadpool-before-start.patch`

```
@@ -76,10 +76,10 @@
     signal(SIGPIPE, SIG_IGN);
     ::android::ProcessState::initWithDriver("/dev/vndbinder");
-    // start a threadpool for vndbinder interactions
-    ::android::ProcessState::self()->startThreadPool();
     ABinderProcess_setThreadPoolMaxThreadCount(1);
+    // start a threadpool for vndbinder interactions
+    ::android::ProcessState::self()->startThreadPool();
     ABinderProcess_startThreadPool();
```

patch aplica con `git apply` en el checkout de `hardware/interfaces`
(case-map: `hardware-interfaces` → `hardware/interfaces`). Idempotente
(git apply con reverse-check antes de aplicar).

## Stopgap (sin rebuild) — dispositivo, TWRP
Mientras no llegue el rebuild, se neutraliza el service en el J2 para que
no vuelva a arrancar ni relanzarse:

1. `/system_root/system/etc/init/audioserver.rc` (system image):
   los 3 triggers `on property:...` tenian `start vendor.audio-hal`;
   se comentaron esas 6 lineas (FIX-034c2). Backup:
   `/system_root/system/etc/init/audioserver.rc.bak-fix034c2`.
2. `/system_root/system/vendor/etc/init/zzz-vendor-audio-hal.disabled.rc`
   (NUEVO, FIX-034c/e): redeclara el service con
   `oneshot` + `disabled` + `override`:
   - `disabled` → `class_start hal` no lo levanta
   - `oneshot` → aunque alguien lo arranque y muera, init NO lo relanza
     (ESTA era la causa del loop persistente: init re-arranca cualquier
     service no-oneshot que muere por SIGABRT)
   - `override` → suplanta la definicion original de `android.hardware.audio.service.rc`
3. `/system_root/system/etc/init/audioserver_no_hal.rc` (previo, FIX-034b):
   override del service `audioserver` con todos los `onrestart`/`start`
   del HAL comentados + triggers `on` sin starts.

Resultado medido: antes del fix 11-16 tombstones de audio por boot; con el
stopgap, como maximo UN solo arranque temprano que muere y ya no se relanza.

## Efecto en el boot (a confirmar)
Con el crash-loop cortado, el sistema avanza a system_server
(se creó `/data/system/environ`). Falta confirmar si la animación aparece.

## Reversión del stopgap (cuando llegue el rebuild con el patch de raíz)
- Borrar `/system_root/system/vendor/etc/init/zzz-vendor-audio-hal.disabled.rc`
- Restaurar `audioserver.rc` desde `audioserver.rc.bak-fix034c2`
- Borrar `audioserver_no_hal.rc`
- Re-flashear un boot/system nuevo (el patch de raíz lo regenera)