# INVESTIGACION GLOBAL FINAL — XNU 4570.71.2 / SAMSUNG J2

## Estado actual
- Rama: experiment/ios-port.
- XNU 4570.71.2 compila y enlaza en CI.
- Errores restantes: symbolsets KPI por armv7k, no bloquean enlace.
- No hay evidencia de arranque real.

## Comprobado
[C] RAM 0x20000000-0x5f4fffff.
[C] GIC, MCT, UARTs confirmados por ADB.
[C] XNU boot_args y Apple DT formato conocido.
[C] Workflow CI exitoso en 2m12s.

## Inferencias
[I] Base candidata 0x80000000/0x80001000.
[I] Se requiere shim S-Boot -> XNU.
[I] Drivers Exynos necesarios para primer arranque.

## Hipótesis
[H] Compatibilidad armv7k con Cortex-A7.
[H] TopOfKernelData exacto.
[H] Conversión FDT -> Apple DT completa.

## Bloqueadores
1. Shim de arranque.
2. topOfKernelData.
3. DTB real.
4. Drivers UART/GIC/MCT.
5. Stubs peligrosos.

## Próximo paso
Calcular layout y generar boot_args/Apple DT mínimo, luego QEMU.
