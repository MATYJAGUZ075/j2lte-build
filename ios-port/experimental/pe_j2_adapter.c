/*
 * J2 (Exynos 3475) Platform Expert adapter for XNU 4570.
 *
 * Implements PE_init_platform using the J2 early drivers.
 * This file is compiled into the XNU kernel for ARM J2.
 */

#include <pexpert/pexpert.h>
#include <pexpert/arm/boot.h>
#include <pexpert/gen/device_tree.h>

/* J2 early driver MMIO bases (from J2.h) */
#define J2_UART0_BASE  0x13800000U
#define J2_GIC_BASE    0x10C01000U
#define J2_MCT_BASE    0x100B0000U

/* Minimal J2 PE state */
static boot_args *j2_boot_args = NULL;

void
PE_init_platform(boolean_t vm_initialized, void *args)
{
	boot_args *ba = (boot_args *)args;

	if (PE_state.initialized == FALSE) {
		PE_state.initialized = TRUE;
		PE_state.bootArgs = ba;
		PE_state.deviceTreeHead = ba->deviceTreeP;
	}

	if (!vm_initialized) {
		DTInit(PE_state.deviceTreeHead);
	} else {
		/* pe_arm_init_interrupts and pe_arm_init_debug would go here */
	}
}
