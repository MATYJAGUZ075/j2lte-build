# Diagnóstico completo J2LTE — 2026-10-26

## Estado de la build

- Dispositivo: Samsung Galaxy J2 LTE (`j2lte`), modelo `SM-J200M`.
- Android: 13.
- Build: `20.0-20261026-UNOFFICIAL-j2lte`.
- Fecha de build: `Mon Oct 26 01:19:05 UTC 2026`.
- Estado: boot completo (`sys.boot_completed=1`), sistema estable.
- Brillo: funcionando. El fix real de la HAL de luz quedó aplicado; no se desactivó el brillo.
- Hora de los logs: hora local del dispositivo (24-oct/25-oct); la build es del 26-oct.
- Diagnóstico realizado por ADB de solo lectura, sin reiniciar ni modificar el dispositivo.

## Resumen de prioridades

| Área | Estado | Causa principal | Acción |
|---|---|---|---|
| Cámara | Rota | La ruta HALv1 no implementa `getCameraCharacteristics` | Implementar metadata HALv1 o fallback API1 |
| Linterna | Rota por arrastre | Falla al pedir características de cámara | Se resuelve con cámara |
| Wi-Fi | Falla autenticación | PSK incorrecta; el AP es WPA2-PSK | Olvidar red y reingresar la contraseña |
| Bluetooth | A2DP no conecta | Timeout de conexión A2DP; stack sin crash | Reproducir con el equipo encendido y capturar HCI |
| Sensores | Sin fallo reproducible ahora | HAL reporta 2 sensores corriendo; no hay clientes | Reproducir auto-rotación/proximidad y capturar logs |
| RAM/lag | Presión de memoria | 1 GB RAM, swap alto, SystemUI/Ajustes pesados | Optimizar memoria, no reintroducir panel eliminado |
| USB | Notificación duplicada | App propia + notificación nativa A13 | Quitar la notificación propia |
| SystemUI | Crash al inicio | NPE en `AppOpsControllerImpl` | Vigilar recurrencia |

---

## 1. Cámara

### Evidencia

`dumpsys media.camera` muestra 2 dispositivos de cámara y 2 cámaras públicas API1, sin clientes activos. Los servicios `cameraserver`, `android.hardware.camera.provider@2.5-service` y `android.hardware.camera.provider@2.4-service` están corriendo.

Error repetido:

```text
getCameraCharacteristics: Function not implemented (-38)
Unable to retrieve camera characteristics for device 0. : 10 (Unknown error -10)
```

Crash real de Aperture:

```text
FATAL EXCEPTION: main
Process: org.lineageos.aperture
CameraAccessExceptionCompat: CAMERA_ERROR (3):
getCameraCharacteristics:791:
Unable to retrieve camera characteristics for device 0:
Function not implemented (-38)
```

Stack relevante:

```text
org.lineageos.aperture.camera.CameraManager.<init>(CameraManager.kt:28)
org.lineageos.aperture.CameraActivity.onCreate(CameraActivity.kt:564)
androidx.camera.camera2.internal.SupportedSurfaceCombination.<init>
android.hardware.camera2.CameraManager.getCameraCharacteristics
```

Ocurrió a las 15:12:00 y 15:12:06; Aperture murió y Android mostró el diálogo de crash.

### Causa

El soporte HALv1 agregado a A13 implementa `getCameraInfo`, pero no `getCameraCharacteristics` para `HidlDeviceInfo1`. La API1 puede enumerar las cámaras, pero Camera2/Aperture falla al pedir la metadata obligatoria.

Los parches activos de cámara están en:

- `patches/frameworks-av/Camera-soporte-HALv1-en-A13.patch`
- `patches/frameworks-av/Camera-soporte-HALv1-fase3.patch`
- `patches/frameworks-av/Camera-soporte-HALv1-fase4-linterna.patch`

En `Camera-soporte-HALv1-en-A13.patch` se ve `getCameraInfo`, pero no una implementación de `getCameraCharacteristics` para la ruta HALv1.

### Fix propuesto

Implementar `getCameraCharacteristics` en `HidlProviderInfo::HidlDeviceInfo1`, construyendo la metadata Camera2 a partir de los parámetros/información del HALv1, o agregar un fallback controlado para que `CameraService` devuelva metadata mínima API1. Después probar:

- Abrir Aperture sin crash.
- `dumpsys media.camera` sin errores de características.
- Encender/apagar linterna.

---

## 2. Linterna

### Evidencia

- `/sys/class/leds` no contiene nodos de linterna/torch.
- El código nuevo de `CameraHardwareInterfaceFlashControl` existe en la fase 4.
- SystemUI falla antes de llegar al control de flash:

```text
E FlashlightController:
android.hardware.camera2.CameraAccessException:
CAMERA_ERROR (3): getCameraCharacteristics:791:
Unable to retrieve camera characteristics for device 0:
Function not implemented (-38)
```

### Causa

La linterna depende de Camera2 para consultar las características del flash. Como esa consulta falla, el toggle nunca llega al HALv1. No hay un LED de linterna independiente en sysfs.

El mensaje de “cámara en uso” es consecuencia de la misma falla; `dumpsys media.camera` no muestra un cliente activo.

### Fix propuesto

No hace falta un LED nuevo. Primero corregir `getCameraCharacteristics`; luego validar `setTorchMode`. Si Aperture sigue fallando, revisar `CameraFlashlight.cpp` y el camino `supportSetTorchMode/HALv1`.

---

## 3. Wi-Fi

### Evidencia

- Wi-Fi habilitado, pero desconectado.
- SSID objetivo: `Fibertel WiFi985 2.4GHz`.
- BSSID: `98:77:e7:d1:0b:ff`, canal 6 (2437 MHz), señal fuerte.
- Scan del AP:

```text
[WPA2-PSK-CCMP+TKIP][RSN-PSK-CCMP+TKIP][WPA-PSK-CCMP+TKIP][ESS][WPS]
```

- Secuencia: `SCANNING -> ASSOCIATING -> ASSOCIATED -> FOUR_WAY_HANDSHAKE`.
- A los ~10 segundos: `AUTHENTICATION_FAILURE_EVENT`, `reasonCode: 3`, `locallyGenerated: true`.
- `WifiMetrics`: `AUTH_FAILURE_TIMEOUT`, `networkType=TYPE_WPA3`, `mAuthentication=2`.

Log decisivo de `wpa_supplicant`:

```text
WPA: 4-Way Handshake failed - pre-shared key may be incorrect
CTRL-EVENT-SSID-TEMP-DISABLED ... reason=WRONG_KEY
```

### Causa

La contraseña/PSK guardada no coincide con la del AP. No es un fallo de netd, DHCP ni de creación de red física. La conexión llega hasta el 4-way handshake y el AP rechaza la clave.

La configuración guardada tiene WPA2-PSK y además parámetros de auto-upgrade a WPA3-SAE, pero el AP anuncia solo WPA2-PSK. `wpa_supplicant` está usando `key_mgmt=0x2` (WPA-PSK), así que el problema inmediato es la PSK, no SAE.

### Fix propuesto

1. Olvidar la red `Fibertel WiFi985 2.4GHz`.
2. Volver a agregarla como WPA2/WPA-PSK con la contraseña exacta.
3. Si sigue fallando, probar sin MAC aleatoria y verificar si el router tiene filtrado MAC o cambió la clave.
4. Recién después investigar driver/firmware.

No tocar netd para este síntoma.

---

## 4. Bluetooth

### Evidencia

- Adaptador `ON`.
- Nombre: `SM-J200M`.
- Dirección: `7C:91:22:DC:3F:D3`.
- Sin crashes de Bluetooth (`Bluetooth crashed 0 times`).
- Dispositivo vinculado: `41:42:AE:27:9B:84 [BR/EDR] JK601`.
- A2DP source habilitado.
- Estado actual: `DISCONNECTED`.

Secuencia A2DP:

```text
15:14:34 CONNECT
15:15:04 CONNECT_TIMEOUT
15:15:04 DISCONNECTED
15:15:04 reintento CONNECT
15:15:08 DISCONNECTED
```

No se observó una conexión A2DP estable. También aparece de forma repetida:

```text
BatteryExternalStatsWorker: error reading Bluetooth stats: 11
```

Ese error de estadísticas es secundario; no explica por sí solo el fallo de audio.

### Causa probable

La vinculación funciona, pero la conexión A2DP no se completa. Puede ser el equipo remoto apagado/fuera de alcance, un problema de perfil A2DP, o un fallo del stack/driver. No hay evidencia suficiente para culpar al parche HCI `774d7c6`; ese cambio solo tolera eventos HCI malformados.

### Próximo paso

Reproducir con el `JK601` encendido, cerca y en modo vinculable, y capturar:

- `dumpsys bluetooth_manager` antes y después.
- `logcat -b all` filtrado por `Bluetooth`, `A2DP`, `HCI`, `bt_`.
- `btsnoop`/`btsnooz_hci.log` si está disponible.

---

## 5. Sensores

### Evidencia actual

`dumpsys sensorservice`:

```text
Total 2 h/w sensors, 2 running 0 disabled clients
K2HH Acceleration
CM36672P Proximity Sensor
0 active connections
```

- Auto-rotación del sistema: habilitada (`accelerometer_rotation=1`).
- `raw_data` del acelerómetro devuelve valores reales: `-575,26,8313`.
- Proximidad responde en sysfs.
- El kernel muestra actividad `k2hh_work_func` y `k2hh_set_mode`.
- No aparece un AVC de SELinux relacionado con sensores; el dispositivo está en `Permissive`.

Se observó una vez:

```text
SensorService: Error activating sensor 0 (Operation not permitted)
```

Pero en el estado actual ambos sensores figuran corriendo y el HAL responde. No hay un fallo actual reproducible en `dumpsys`.

### Interpretación

El hardware y el driver parecen vivos. Si el usuario ve que no funciona la auto-rotación o la proximidad, hay que reproducirlo con una app que registre el sensor y capturar el momento exacto. No conviene afirmar que el sensor está roto solo por el error histórico `EPERM`.

---

## 6. RAM y lag

### Evidencia

```text
Mem: 866588K total, 791868K used, 74720K free
Swap: 1048572K total, 326552K used, 722020K free
Load: 2.3 / 2.66 / 2.56
```

RSS principales:

```text
204 MB  com.android.systemui
152 MB  system_server
130 MB  com.android.settings
100 MB  com.android.launcher3
98 MB   surfaceflinger
```

CPU: no hay un proceso acaparando CPU de forma sostenida; Ajustes ~3.8 %, `system_server` ~3.5 %, SurfaceFlinger ~2.3 %, SystemUI ~1.5 %.

### Interpretación

El equipo tiene 1 GB de RAM y está usando swap de forma agresiva. El lag es principalmente presión de memoria y swapping, no un bucle de CPU.

Los cambios posteriores al 16-oct no agregaron una optimización real de RAM: el panel “Optimizaciones” fue revertido en `9115e60`; el trabajo de esa área quedó anulado. No reintroducir ese panel.

### Próximo paso

- Medir PSS real con `dumpsys meminfo` en uso normal.
- Revisar zram/LMK y la cantidad de apps persistentes.
- Reducir el costo de SystemUI/Ajustes antes de agregar más “tweaks”.

---

## 7. Notificación USB

### Evidencia

Hay dos notificaciones activas al mismo tiempo:

1. Notificación propia:

```text
pkg=com.android.settings.j2lte
id=4242
channel=j2lte-usb
title=USB conectado
text=Toca para cambiar la funcion USB
actions=4 (MTP, PTP, Solo ADB, Solo carga)
```

2. Notificación nativa de Android 13:

```text
pkg=android
id=27
channel=USB
title=Se activó la transferencia de archivos mediante USB
text=Presiona para ver más opciones.
```

El paquete propio está instalado en:

```text
/system/priv-app/j2lte-usb/j2lte-usb.apk
```

El código está en:

```text
devtree-j2lte/Usb/src/com/android/settings/j2lte/UsbFunctionActivity.java
devtree-j2lte/Usb/AndroidManifest.xml
devtree-j2lte/Usb/Android.bp
```

Y se incluye en `devtree-j2lte/device.mk` mediante `PRODUCT_PACKAGES += j2lte-usb`.

Estado USB actual:

```text
connected=true
configured=true
current_functions=MTP
current_functions_applied=true
```

### Conclusión

La notificación propia es redundante: A13 ya muestra la notificación nativa y ofrece acceso a las opciones USB. La app propia sigue siendo útil solo si se quiere conservar su pantalla para cambiar funciones.

### Opciones

- Mínima: dejar de publicar la notificación propia, conservando la app para el selector de funciones USB.
- Completa: quitar `j2lte-usb` de `PRODUCT_PACKAGES`, eliminar la app y su allowlist, y validar que la notificación nativa permita cambiar MTP/PTP/ADB/carga.

---

## 8. Hallazgo colateral: crash de SystemUI al inicio

A las 15:00:08 SystemUI crasheó al crear `KeyguardService`:

```text
java.lang.NullPointerException:
Attempt to invoke interface method 'java.util.Iterator java.util.List.iterator()'
on a null object reference
at com.android.systemui.appops.AppOpsControllerImpl.setListening
```

SystemUI se recuperó y siguió funcionando. No se repitió en el buffer actual. Puede ser un fallo de arranque aislado, pero conviene vigilarlo porque afecta notificaciones, pantalla de bloqueo y rendimiento percibido.

---

## 9. Cambios posteriores al 16-oct

Se revisaron 27 commits entre el 16-oct y el 26-oct. Se agrupan así:

- Cámara HALv1 en A13: `7281dee`, `180d69b`, `1c3b1fa`, `cb2f3e2`, `40f966b`, `5a41aa6`, `f4809b9`, `5dd6e0f`, `68e9e95`.
- Luz/brillo: `ef36dbe`, `fc5714d`, `a0192a6`.
- QR/CPU: `2168e5b`, `0ea12e7`.
- Ajustes/Optimizaciones: `7773378` (agregado) y `9115e60` (revertido; estado final sin panel).
- USB: `75e5a1c`, `b6e0df0`.
- Bluetooth: `774d7c6` (tolerancia a eventos HCI malformados).
- Netd: `0ca0d0c`, `b14e1fe`.
- TWRP/CI: `d9217cc`, `692c323`, `e8a1ef2`.
- Adbd: `68e1611`.
- Allowlist: `abaa935`, `adee86c`.

No hay cambios post-16-oct que expliquen directamente el fallo de autenticación Wi-Fi, el timeout A2DP ni los sensores. El panel “Optimizaciones” no está en la build actual y no debe reintroducirse.

---

## 10. Próximos pasos concretos

1. Cámara/linterna: implementar `getCameraCharacteristics` para HALv1 y probar Aperture + torch.
2. Wi-Fi: olvidar la red y reingresar la PSK; no tocar netd.
3. Bluetooth: reproducir con el `JK601` encendido y capturar HCI/A2DP.
4. Sensores: reproducir auto-rotación/proximidad; solo entonces buscar el `EPERM`.
5. USB: quitar la notificación propia; decidir si se conserva la app de funciones.
6. RAM: medir PSS/LMK/zram; no volver a agregar el panel eliminado.
7. SystemUI: vigilar si el NPE de `AppOpsControllerImpl` reaparece.

## Archivos de referencia

- `/tmp/opencode/j2lte-build/patches/frameworks-av/Camera-soporte-HALv1-en-A13.patch`
- `/tmp/opencode/j2lte-build/patches/frameworks-av/Camera-soporte-HALv1-fase4-linterna.patch`
- `/tmp/opencode/devtree-j2lte/Usb/src/com/android/settings/j2lte/UsbFunctionActivity.java`
- `/tmp/opencode/devtree-j2lte/Usb/AndroidManifest.xml`
- `/tmp/opencode/devtree-j2lte/device.mk`
- `/tmp/opencode/j2lte-build/docs/ANOMALIAS-BUGS-J2-2026-09-29.md`
- `/tmp/opencode/DIAGNOSTICO-SUBSISTEMAS-2026-10-19.md`
- `/tmp/opencode/DIAGNOSTICO-USB-Y-OTROS-2026-10-19.md`
