# j2lte-build (One UI 5 experiment)

LineageOS 20 disfrazado de One UI 5: experimento local para probar cambios cosméticos en forks de `frameworks/base` y `packages/apps/Settings`.

Este no es un producto Samsung. Es una ROM basada en LineageOS 20 / Android 13 para el Galaxy J2 (j2lte, Exynos 3475, ARM32, 1 GB RAM).

---

# Workflows

- "los20-sync-test.yml" — Syncs the LineageOS 20 sources and checks storage usage.
- "los20-lunch-check.yml" — Prepares the build environment and checks the "lineage_j2lte" target.
- "los20-build-module.yml" — Builds individual modules with "mka" for faster testing.
- "los20-build-full.yml" — Full LineageOS 20 build.
- "los20-build-stages.yml" — Staged build workflow using the reduced manifest to keep storage usage under control.
- "los20-build-diag.yml" — Diagnostic workflow for investigating build issues.

---

# Manifests

The main manifests are located in "manifests/".

- "j2lte-slim.xml" — Reduced manifest currently used for CI builds where storage is limited.
- "j2lte.xml" — Full J2 manifest, now pointing to local forks for frameworks/base and packages/apps/Settings.
- "j2lte-slsi.xml" — Variant using the Samsung SLSI Exynos fork.

The manifests point to:

- Device tree
- Universal3475 common device tree
- Exynos3475 kernel
- Samsung vendor tree
- Samsung SLSI hardware components
- Local fork of frameworks/base (One UI color accents)
- Local fork of packages/apps/Settings (One UI blue accent)

---

# Build

The main build target is:

lunch lineage_j2lte-userdebug
mka bacon

Individual modules can also be built through "los20-build-module.yml".

---

# Current Status

Este repo está en la rama experimental `experiment/oneui5`. Los cambios son mínimos y específicos de colores/es:

- `frameworks/base` → `MATYJAGUZ075/android_frameworks_base-1`, branch `lineage-20.0`
- `packages/apps/Settings` → `MATYJAGUZ075/android_packages_apps_Settings-1`, branch `lineage-20.0`

No se subió aún esta rama de `j2lte-build` a GitHub. Solo se actualizaron los manifiestos localmente.
