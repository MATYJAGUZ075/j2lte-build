# S-BOOT TO XNU HANDOFF SPEC

## XNU _start requirements
[C] start.s: r0 = PA boot_args, r1 = 0.
[C] Mode ARM, MMU off, D-cache off, I-cache on.
[C] Reads physBase/virtBase/memSize/topOfKernelData from boot_args.
[C] Creates page tables and enables MMU.

## S-Boot evidence
[C] S-Boot prints Starting kernel at 0x20008000.
[C] ATAG_MEM 4 regions.
[C] ATAG_INITRD2 0x22000000.
[C] No DTB appended message.

## Comparison
| Elemento | S-Boot/J2 | XNU | Estado |
| Registros | Investigar | r0=PA boot_args | Pendiente |
| MMU | Linux espera off | off | Compatible |
| Cachés | Investigar | D off, I on | Pendiente |
| boot_args | No entrega | Requiere | Shim necesario |
| Device Tree | FDT/ATAG | Apple DT | Shim necesario |

## Shim design
[I] Preservar memoria, construir boot_args, generar Apple DT, saltar en ARM mode.

## Pendientes
[U] Registros reales de S-Boot.
[U] topOfKernelData.
[U] DTB entrega.
