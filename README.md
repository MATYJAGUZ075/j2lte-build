# j2lte-build

GitHub Actions setup for building and testing the LineageOS 20 port for the Samsung Galaxy J2 (j2lte / Exynos3475).

This repository contains the manifests and workflows used to sync the Android source tree, run diagnostics and build LineageOS 20 on GitHub Actions.

---

# Workflows

- "los20-sync-test.yml" — Syncs the LineageOS 20 sources and checks storage usage.
- "los20-lunch-check.yml" — Prepares the build environment and checks the "lineage_j2lte" target.
- "los20-build-module.yml" — Builds individual modules with "mka" for faster testing.
- "los20-build-full.yml" — Full LineageOS 20 build.
- "los20-build-stages.yml" — Staged build workflow using the reduced manifest to keep storage usage under control.
- "los20-build-diag.yml" — Diagnostic workflow for investigating build issues.

Most workflows can be started manually from:

Actions → Select a workflow → Run workflow

---

# Manifests

The main manifests are located in "manifests/".

- "j2lte-slim.xml" — Reduced manifest currently used for CI builds where storage is limited.
- "j2lte.xml" — Full J2 manifest.
- "j2lte-slsi.xml" — Variant using the Samsung SLSI Exynos fork.

The manifests point to the repositories used by the J2 port, including:

- Device tree
- Universal3475 common device tree
- Exynos3475 kernel
- Samsung vendor tree
- Samsung SLSI hardware components

---

# Build

The main build target is:

lunch lineage_j2lte-userdebug
mka bacon

Individual modules can also be built through "los20-build-module.yml" when a full build is unnecessary.

---

# Current Status

LineageOS 20 for the Samsung Galaxy J2 (SM-J200M / j2lte) is still under development.

The project is being built and debugged through GitHub Actions. The reduced manifest and staged workflows are used to keep the source tree within the storage limits of the CI runners.

This repository is mainly for the build infrastructure; the actual device, kernel and vendor sources are kept in their respective repositories.
