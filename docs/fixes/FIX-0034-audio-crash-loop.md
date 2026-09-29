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
no vuelva a arrancar ni relanzarse.

### Ensayos que NO funcionaron (importante, no repetir)
1. `disabled` + `override` en un rc aparte: NO basta. `disabled` solo frena
   `class_start hal`; los `start vendor.audio-hal` EXPLICITOS (triggers
   `on property:` de audioserver, `onrestart` de audioserver) lo levantan igual.
2. `oneshot` + `disabled` + `override`: NO sirve. **init IGNORA `oneshot` para
   procesos muertos por SEÑAL (SIGABRT)** — la opcion `oneshot` solo evita el
   reinicio cuando el proceso sale limpio (exit). Un crash con señal SIEMPRE
   se relanza por init, generando el crash-loop.

### Solucion efectiva (FIX-034f)
ELIMINAR la definicion del service en el rc original que init SI lee:
`/system_root/system/vendor/etc/init/android.hardware.audio.service.rc`:
TODO el bloque `service vendor.audio-hal ...` esta comentado (0 lineas
activas). Sin definicion de service, cualquier `start/restart
vendor.audio-hal` que ejecute init es un no-op ("no such service"): no hay
proceso que crashee, no hay tombstones, el boot continua.

Defensa en profundidad (se mantiene):
- `/system_root/system/etc/init/audioserver.rc` (system image): los 3 triggers
  `on property:...` tenian `start vendor.audio-hal`; se comentaron esas lineas
  (FIX-034c2). Backup: `audioserver.rc.bak-fix034c2`.
- `/system_root/system/etc/init/audioserver_no_hal.rc` (FIX-034b): override del
  service `audioserver` con los `onrestart`/`start` del HAL comentados.
- `zzz-vendor-audio-hal.disabled.rc` movido a `.off` (init no lo parsea) para
  no reintroducir una definicion espuria del service.

Backups: `android.hardware.audio.service.rc.bak-fix034f` (el rc original
intacto, 517 B), `audioserver.rc.bak-fix034c2`,
`android.hardware.audio.service.rc.bak-fix034c`.

Resultado medido: antes del fix 11-16+ tombstones de audio por boot (loop con
pids crecientes). Con el service eliminado NO puede haber ningun tombstone del
HAL: el service no existe.

## Efecto en el boot (a confirmar)
Con el crash-loop cortado, el sistema avanza a system_server
(se creó `/data/system/environ`). Falta confirmar si la animación aparece.

## Reversión del stopgap (cuando llegue el rebuild con el patch de raíz)
- Restaurar `android.hardware.audio.service.rc` desde
  `android.hardware.audio.service.rc.bak-fix034f`
- Borrar `zzz-vendor-audio-hal.disabled.rc.off`
- Restaurar `audioserver.rc` desde `audioserver.rc.bak-fix034c2`
- Borrar `audioserver_no_hal.rc`
- Re-flashear un boot/system nuevo (el patch de raíz lo regenera)