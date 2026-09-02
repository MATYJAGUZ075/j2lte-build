# Registro de podas de prebuilts (j2lte-build)

Historial de intentos de poda de prebuilts para controlar el uso de espacio en CI
(GitHub Actions). Documenta qué se probó, la evidencia obtenida y el resultado.

> Regla general: **no asumir que una poda es segura por análisis estático o por
> `repo manifest`.** La evidencia definitiva es el build real (Soong bootstrap,
> mka, etc.). Una poda solo debe mantenerse si el build completo termina OK.

---

## FALLIDA — `prebuilts/sdk/1..27` (~688 MB)

- **Status:** FALLIDA (revertida). No reintentar sin evidencia de build que lo justifique.
- **Commit de prueba:** `5987251` (poda aplicada en los 2 workflows).
- **Commit de reversión:** revert aplicado sobre `5987251` (workflows restaurados a `7a43e69`).
- **Evidencia:** la auditoría estática/`repo manifest` dio **falso positivo** de
  seguridad, pero el **build real falló en Soong bootstrap (~99%)** con referencias
  reales a los `android.jar` antiguos:
  - `prebuilts/sdk/15/public/android.jar` → `android-support-multidex`
  - `prebuilts/sdk/9/public/android.jar`  → `mp4parser`
  - `prebuilts/sdk/4/public/android.jar`  → `dexgen`
  - `prebuilts/sdk/8/public/android.jar`  → `oauth`
  - `prebuilts/sdk/19/public/android.jar` → `crcalc`
  - `prebuilts/sdk/9/public/android.jar`  → `easymock`
- **Lección:** a pesar de que `aosp_arm`/j2lte usa SDK 28..33/current para el
  sistema, módulos de host/build (multidex, mp4parser, oauth, etc.) dependen de
  los `android.jar` **antiguos** (1..27) durante el bootstrap de Soong. Esos
  stubs SÍ son necesarios. No se deben podar.
- **NO se revirtió** la poda de `android-emulator` + `remoteexecution-client`
  (esa sigue activa y es independiente de este fallo).
- **No se tocó** `.repo`, JDK, GCC, abi-dumps, vndk, rust, cts, test, development,
  device tree, kernel, FIX-028 ni FIX-029.

---

## Referencia (podas previas)

- `android-emulator` + `remoteexecution-client` (post-sync): mantenida. En ambos
  workflows se hace tolerante (si el manifest slim ya las elimina via
  `<remove-project>`, se informa y continúa).
