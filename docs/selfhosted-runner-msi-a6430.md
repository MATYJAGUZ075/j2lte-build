# Runner self-hosted en la MSI A6430 (2011) + WSL

Contexto: los runners hosted de GitHub bajaron a ~72-75G de disco; el full-bacon
necesita ~133G de pico. Los runners de 145G todavia existen en el pool (la stages
del 28/09 11:22Z agarro uno), pero es ruleta. Este doc es el plan B: correr el
build en la laptop via un runner self-hosted de GitHub Actions.

Especificaciones de la maquina (reales, no inventadas):
- MSI A6430, i3-2330M (2 nucleos / 4 hilos, 2da gen)
- 4 GB de RAM
- Disco D: SSD 512 GB (mismo disco de Windows)
- Windows 10 1809 + WSL (levantado en D: para tener espacio)

ADVERTENCIA HONESTA:
- Un full-bacon en runner hosted 4vCPU/16GB toma ~5h31m. Esta maquina (4GB RAM,
  WSL sobre disco compartido con Windows) va a swapear muchisimo: estimado 15-40h.
  Es lento pero viable TECNICAMENTE. Ajusta la RAM con swapfile.
- Boot diag (pstore/boot_diag=true) es imprescindible SIEMPRE en esta laptop
  porque el device aun no completa boot.

## 1. Preparar WSL (si no esta instalado)

En PowerShell como admin:

    wsl --install
    # en 1809 usa: dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
    # despues instala "Ubuntu" desde Microsoft Store (o ubuntu-22.04 / 24.04)
    # (O si WSL2 no esta disponible en 1809, WSL1 tambien sirve para correr el runner)

La distribucion debe quedar en D: para no llenar C:. Opcion: mover el vhdx del
distro a D:\WSL\Ubuntu\ext4.vhdx (WSL2) o instalar el distro con --location D:\WSL.

## 2. Dentro del WSL: prerequisitos del runner

    sudo apt update && sudo apt install -y git curl python3 ca-certificates build-essential libssl-dev

Herramientas del build de LOS (repo ya lo baja el workflow, pero conviene tener git):

    git config --global user.name "Matyjaguz07"
    git config --global user.email <tu-email>

Swapfile para 4GB de RAM: minimo 8-12GB (si o si):

    # agrega swap temporal (SOBREVIVE al reboot en WSL2 no siempre; rehacer tras reboot)
    sudo fallocate -l 12G /swapfile
    sudo chmod 600 /swapfile
    sudo mkswap /swapfile
    sudo swapon /swapfile
    # verificar:
    free -h

## 3. Instalar el runner de GitHub Actions

Ir a: Settings > Actions > Runners > New self-hosted runner
- Seleccionar Linux x64
- Copiar el comando de descarga y creacion. Ejemplo:

    mkdir -p /home/<user>/actions-runner && cd /home/<user>/actions-runner
    curl -o actions-runner-linux-x64-<ver>.tar.gz -L <url-del-archivo>
    tar xzf ./actions-runner-linux-x64-<ver>.tar.gz

Registrar con labels util (importante: el workflow usa `runner_label`):

    ./config.sh --url https://github.com/MATYJAGUZ075/j2lte-build \
      --token <TOKEN> --name msi-a6430 --labels msi-a6430,self-hosted,linux,x64

    # ejecutar el runner (bloqueante):
    ./run.sh

    # o correrlo como servicio:
    sudo ./svc.sh install && sudo ./svc.sh start

Finalizar:
    ./config.sh --remove --token <TOKEN>

## 4. Lanzar el full desde la laptop (Kernel locale)

El workflow tiene input `runner_label` y `min_root_gb`. Para correr en el runner
de la laptop:

    gh workflow run los20-build-full.yml \
      -f runner_label=msi-a6430 \
      -f mode=full-bacon \
      -f boot_diag=true \
      -f pstore_diag=true \
      -f mka_jobs=1 \
      -f min_free_gb=40 \
      -f min_root_gb=100

Explicacion de los parametros para 4GB RAM:
- `mka_jobs=1`: con 4GB, un solo job de compilacion. (2 riskea OOM.)
- `min_free_gb=40`: post-sync pedis 40G libres para permitir bacon (rigido).
- `min_root_gb=100`: el disco D: da 512G, pasa tranquilo el guard temprano.
- boot/pstore_diag=true: diagnostico de arranque (el device aun no completa boot).

## 5. Monitorear

    gh run watch
    gh run view <id> --log | grep -a "mka bacon" 

## 6. Notas de RAM / rendimiento en esta laptop

- Nunca suites a mka_jobs>1 sin vigilar `free -h`.
- El disco compartido con Windows rechina con I/O; no abras mucho en Windows
  durante el build.
- Si Windows usa ~40-60% de RAM por si solo, cierra apps antes de lanzar.
- WSL1 vs WSL2: en 1809 WSL2 puede no estar disponible. Ambos corren el runner;
  WSL2 es mas rapido para build. Si no hay WSL2, WSL1 funciona pero mas lento.

## 7. Seguridad

- El runner de un repo publico puede ejecutar code arbitrario de PRs → NO
  habilites auto-dispatch de PRs; lanza solo manual con workflow_dispatch.
- Considera `runs-on` fijo a tu label para que nadie mas lo use.