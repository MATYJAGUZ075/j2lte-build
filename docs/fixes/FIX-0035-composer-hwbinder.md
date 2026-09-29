# FIX-0035 — Composer transport `hwbinder` (surfaceflinger no encuentra hwcomposer)

## Problema
LOS20 A13 en J2: surfaceflinger aborta con
`failed to get hwcomposer service`. Tombstones de surfaceflinger
(`tombstone_09`, 15:14) antes de aplicar este fix.

## Causa raíz
El manifest del device declara `android.hardware.graphics.composer@2.1` con
`transport passthrough`. Con passthrough el framework carga la implementación
en su propio proceso; cuando el device solo trae la variante 2.1 normal
(hwbinder), el registro del HAL falla y surfaceflinger no encuentra el service.

## Fix — GITHUB (ya commiteado y pusheado)
Repo `MATYJAGUZ075/android_device_samsung_j2lte`, branch `lineage-17.1`,
commit `d0709f4` (URL:
`https://github.com/MATYJAGUZ075/android_device_samsung_j2lte/commit/d0709f47d741595bd2eaec4e2e352fd83017d957`)

`manifest.xml`:

```
-    <transport>passthrough</transport>   (dentro de composer@2.1)
+    <transport>hwbinder</transport>
```

## Stopgap (sin rebuild) — dispositivo, TWRP
Sin dar rollback, en el J2 se editó en caliente el vintf del SISTEMA:
`/system_root/system/vendor/etc/vintf/manifest.xml`
(6962 B, md5 e4094565…) — mismo cambio (`hwbinder`).
Backup: `/system_root/system/vendor/etc/vintf/manifest.xml.bak-composer-hwbinder`

Esto persistió tras reboots del dispositivo (verificado remontando después).

## Resultado medido
Desde el fix, ningún tombstone más de surfaceflinger. El error de hwcomposer
desapareció y el sistema avanza (el bloqueo siguiente fue el crash-loop de
audio, ver FIX-0034).

## Reversión
- Red dispositivo (GitHub): revertir `d0709f4` en `lineage-17.1`.
- Dispositivo: restaurar `manifest.xml.bak-composer-hwbinder`.
- Un rebuild regenera `/system/vendor/etc/vintf/manifest.xml` desde el
  device tree (si el commit de GitHub está, sale hwbinder automático).