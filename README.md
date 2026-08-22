# j2lte-build

Infraestructura de GitHub Actions para el porte de LineageOS 20 al
Samsung Galaxy J2 (j2lte / Exynos3475).

- `manifests/j2lte.xml` — local_manifest de repo con nuestros forks
  (device j2lte, universal3475-common, kernel exynos3475, vendor samsung).
- `.github/workflows/los20-sync-test.yml` — prueba de sincronización y
  medición de almacenamiento de las fuentes LineageOS 20 (sin build).

Ejecutar manualmente: Actions → "LOS20 sync test" → Run workflow.
