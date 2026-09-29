# FIX-0036 — `libcamera_client_shim.so` ausente (tumba zygote y bootanimation)

## Problema
Tras el crash-loop de audio (FIX-0034) y el transporte de composer
(FIX-0035), el sistema seguía sin llegar a animación de arranque. El J2 se
quedaba en el logo de Samsung: `init.svc.zygote=[restarting]`,
`init.svc.bootanim=[stopped]`, sin `sys.boot_completed`.

## Causa raíz
No es el HAL de audio. En el log del último arranque
(`/data/local/tmp/bootlog.txt`, 16:31, build `eng.runner.20260928.185449`)
aparecen 45 `CANNOT LINK`:

```
F linker : CANNOT LINK EXECUTABLE "/system/bin/bootanimation":
  library "/vendor/lib/libcamera_client_shim.so" not found:
  needed by /system/lib/libcamera_client.so in namespace (default)
```

Los binarios caídos son precisamente los que impiden el arranque completo:
- `app_process`    → zygote nunca levanta (`init.svc.zygote=restarting`)
- `bootanimation`  → sin animación de arranque
- `audioserver`, `mediaserver`, `cameraserver`, `mediaextractor`

La librería `libcamera_client_shim.so` **no existe** en la imagen
(`find /system_root -name 'libcamera_client_shim*'` → 0 resultados).

FIX-002 (22/08) retiró `libcamera_client_shim` de `PRODUCT_PACKAGES`
suponiendo que «no tenía fuente ni blob → missing module». **El supuesto era
incorrecto**: el módulo sí tiene fuente completa
(`libshims/libcamera_client/CameraParameters.{cpp,h}`) y compila limpio
(solo arrays `const char`, sin dependencias externas). Al excluirlo, la build
nunca lo empaqueta en `/vendor/lib`, pero el linker lo exige en runtime.

El shim exporta las constantes `CameraParameters` que los blobs de cámara de
Samsung resuelven y que la `libcamera_client` de AOSP **no** define:
`KEY_PHASE_AF`, `KEY_RT_HDR`, `METERING_CENTER`, `KEY_DYNAMIC_RANGE_CONTROL`,
`PIXEL_FORMAT_YUV420SP_NV21`, `EFFECT_CARTOONIZE`, etc. (verificado: 0
ocurrencias de esos símbolos en la `libcamera_client.so` de AOSP).

## Fix — GITHUB (commiteado y pusheado)
Repo `MATYJAGUZ075/android_device_samsung_universal3475-common`, branch
`lineage-17.1`, commit `c0327a0` (URL:
`https://github.com/MATYJAGUZ075/android_device_samsung_universal3475-common/commit/c0327a0`)

`device-common.mk`:
```
 PRODUCT_PACKAGES += \
     libstagefright_shim \
+    libcamera_client_shim \
     libgpsd_shim \
     libhardware_legacy
```

`libexynoscamera_shim` y `libui_shim` **siguen fuera**: continúan sin fuente.

El manifest del build (`manifests/j2lte.xml`) consume
`android_device_samsung_universal3475-common` en `lineage-17.1`, así que la
próxima build toma el shim automáticamente (sin tocar el manifest).

## Validación local (pre-build)
- El módulo compila aislado sin errores: `g++ -shared -fPIC` sobre
  `CameraParameters.cpp` produce el `.so` y exporta los símbolos mangled
  correctos (`_ZN7android16CameraParameters12KEY_PHASE_AFE`, etc.).
- `all-makefiles-under` (Android.mk raíz) incluye
  `libshims/libcamera_client/Android.mk` automáticamente, y está gateado por
  `TARGET_DEVICE ∈ {j1xlte, j2lte, on5ltetmo}`.
- `LOCAL_PROPRIETARY_MODULE := true` ⇒ se instala en `/vendor/lib`, que es
  exactamente la ruta que el linker busca.

## Pendiente
Requiere rebuild. Con la build nueva debería desaparecer el `CANNOT LINK` y
zygote / bootanimation deberían levantar. Si aun así no arranca, el siguiente
bloqueo sería otro — este es el primero de la cadena de enlazado.

## Reversión
- Red (GitHub): revertir `c0327a0` en `lineage-17.1`.
