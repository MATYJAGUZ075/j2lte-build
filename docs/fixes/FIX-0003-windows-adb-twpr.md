# FIX-0003 — Windows: adb invisible en TWRP (SM-J200M / j2lte)

## Problema
El J2 en TWRP/recovery enumera en Windows como dispositivo Google
`USB\VID_18D1&PID_4EE2` (ID de proveedor "ACER" por el INF de oem110) y
`adb devices` sale VACÍO, tanto con adb viejo como moderno:

```
List of devices attached
(empty)
```

En cambio, booteando el sistema (LOS14, gadget Samsung `04E8:6860`) adb funciona
perfectamente. El problema es EXCLUSIVO del gadget AOSP de recovery.

## Causa raíz
El INF instalado `C:\Windows\INF\oem110.inf` (Google android_winusb, version
7.0.0.00001, ClassGuid `{3F966BD9-FA04-4ec5-991C-D326973B5128}`) tiene DOS
entradas para `18D1:4EE2`:

```
USB\VID_18D1&PID_4EE2          <-- compuesto ENTERO (sin &MI_)
USB\VID_18D1&PID_4EE2&MI_01    <-- solo la interfaz ADB
```

Windows eligió la del **compuesto entero** → bindería WinUSB a nivel del
dispositivo completo, NO divide las interfaces, y el nodo resultante:

- FriendlyName: `ACER Composite ADB Interface`
- Service: `WinUSB`
- SymbolicName: `\??\USB#VID_18D1&PID_4EE2#42006866d09ba26f#{a5dcbf10-6530-11d2-901f-00c04fb951ed}`
  (GUID genérico USB_DEVICE, NO el GUID ADB)
- DeviceInterfaceGUIDs registrado: `{F72FE0D4-CBCB-407d-8814-9ED673D0DD6B}`

Aunque el DeviceInterfaceGUIDs esté en Device Parameters, el nodo NO publica la
interfaz ADB (la enumeración por SetupDiEnumDeviceInterfaces con el GUID ADB no
devuelve nada para este PID), así que adb no encuentra endpoints para abrir.

La solución es hacer que Windows use **usbccgp (USB Composite Device)** como
driver del PADRE, de modo que divida el dispositivo en sus interfaces hijas:

```
USB\VID_18D1&PID_4EE2\6&11CD9AC8&0&5        "Dispositivo compuesto USB"   usbccgp   (padre)
USB\VID_18D1&PID_4EE2&MI_00\...             "j2lte" (WPD/MTP)            WUDFWpdMtp
USB\VID_18D1&PID_4EE2&MI_01\...             "Android Composite ADB Interface"  WinUSB  <-- ADB
USB\VID_18D1&PID_4EE2&MI_03\...             "SAMSUNG_Android"            WinUSB
```

## Síntomas de diagnóstico
`Get-PnpDevice` mostraba nodos FANTASMA de una división previa correcta:

```
Unknown AndroidUsbDeviceClass Android Composite ADB Interface WinUSB USB\VID_18D1&PID_4EE2&MI_01\7&F6EFC23&0&0001
Unknown WPD                   j2lte                           WUDFWpdMtp USB\VID_18D1&PID_4EE2&MI_00\7&F6EFC23&0&0000
Unknown USBDevice             SAMSUNG_Android                 WINUSB     USB\VID_18D1&PID_4EE2&MI_03\7&F6EFC23&0&0003
```

Eso prueba que el dispositivo SÍ se puede dividir (las interfaces existen); solo
falta que Windows elija usbccgp en lugar del binder global.

## Solución (Manual, desde Admin. de dispositivos)
1. Con el teléfono CONECTADO en TWRP:
2. `Win+X` → Administrador de dispositivos
3. Clic derecho sobre **ACER Composite ADB Interface** →
   **Actualizar controlador**
4. **Buscar controladores en mi equipo**
5. **Elegir de una lista de controladores disponibles** (desmarcar "Mostrar hardware compatible")
6. **Dispositivos de sistema** → **USB Composite Device** → Siguiente/Aceptar
7. El PADRE dará **Código 10 ("Este dispositivo no se puede iniciar")** →
   **IGNORARLO**. No es un error de los hijos.
8. NO desinstalar nada. Verificar que aparecen `MI_01` y `MI_00`/`MI_03`.

### Nota crítica
El PADRE queda con código 10, pero los HIJOS se crean y funcionan. La limpieza
de los nodos (CM_Uninstall) durante el diagnóstico CANCELÓ la división y volvió
a dejar el binder compuesto-entero. **NO desinstalar los hijos ni el padre**
después del paso 6.

## Resultado verificado
Con el split activo, ambos adb funcionan:

```
adb devices -l
42006866d09ba26f       recovery product:omni_j2lte model:j2lte device:j2lte transport_id:1
adb shell getprop ro.build.version.release → 5.1.1
adb shell ls /tmp → recovery.log twadbfifo
```

## Prevención (si vuelve a cortarse)
Script `fix-adb-j2.bat` (en `C:\Users\Usuario\Downloads\`):
mata el server adb, desinstala el nodo `USB\VID_18D1&PID_4EE2\42006866D09BA26F`,
rescan, y reinicia adb. Si tras reconectar sigue como "ACER Composite ADB
Interface" en una sola línea, repetir los pasos 3-6.

## Nota drivers
- El driver Samsung (programa de Samsung para `04E8`) NO aplica a TWRP:
  en recovery el VID es `18D1` (AOSP). No instalarlo.
- En el DriverStore hay DOS copias de android_winusb: `361ab03567fd05bc`
  (Google 2013, ClassGuid `...5128`, con entradas `4EE2` y `4EE2&MI_01`)
  y `7036ed8ea7ec0fa9` (LeMobile 2016, ClassGuid `...B0E`, solo `&MI_01`).
  La instalada (oem110) es la de Google.

## Archivos/servicios clave en Windows
- `C:\Windows\INF\oem110.inf` — INF instalado (Google android_winusb 2013)
- `C:\Windows\System32\DriverStore\FileRepository\usb.inf_amd64_6305f5da0919ba89\usb.inf` — driver "USB Composite Device" (usbccgp)
- adb: `C:\Program Files\Software Fix\adb.exe` (Windows x86, backend WinUSB)