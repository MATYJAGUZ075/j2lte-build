/*
 * J2 (Exynos 3475) Platform Expert - PE_init_platform
 *
 * Replaces pexpert/arm/pe_init.c for the J2 machine config.
 * Integrates J2 early drivers (UART, GIC, MCT) with XNU PE interfaces.
 */

#include <sys/types.h>
#include <sys/kdebug.h>
#include <mach/vm_param.h>
#include <pexpert/protos.h>
#include <pexpert/pexpert.h>

/* PE state definition (was in original pe_init.c) */
PE_state_t PE_state;
#include <pexpert/boot.h>
#include <pexpert/device_tree.h>
#include <pexpert/pe_images.h>
#include <kern/sched_prim.h>
#include <machine/machine_routines.h>
#include <arm/caches_internal.h>
#include <kern/debug.h>
#include <libkern/section_keywords.h>

/* J2 early driver MMIO bases (from J2.h, verified via /proc/iomem) */
#define J2_UART0_BASE  0x13800000U
#define J2_GIC_BASE    0x10C01000U
#define J2_MCT_BASE    0x100B0000U

void
PE_init_platform(boolean_t vm_initialized, void *args)
{
	boot_args *boot_args_ptr = (boot_args *) args;

	if (PE_state.initialized == FALSE) {
		PE_state.initialized = TRUE;
		PE_state.bootArgs = boot_args_ptr;
		PE_state.deviceTreeHead = boot_args_ptr->deviceTreeP;
		PE_state.video.v_baseAddr = boot_args_ptr->Video.v_baseAddr;
		PE_state.video.v_rowBytes = boot_args_ptr->Video.v_rowBytes;
		PE_state.video.v_width = boot_args_ptr->Video.v_width;
		PE_state.video.v_height = boot_args_ptr->Video.v_height;
		PE_state.video.v_depth = (boot_args_ptr->Video.v_depth >> kBootVideoDepthDepthShift) & kBootVideoDepthMask;
		PE_state.video.v_rotate = (boot_args_ptr->Video.v_depth >> kBootVideoDepthRotateShift) & kBootVideoDepthMask;
		PE_state.video.v_scale = ((boot_args_ptr->Video.v_depth >> kBootVideoDepthScaleShift) & kBootVideoDepthMask) + 1;
		PE_state.video.v_display = boot_args_ptr->Video.v_display;
	}

	if (!vm_initialized) {
		DTInit(PE_state.deviceTreeHead);
		pe_init_debug();
	} else {
		/* pe_arm_init_interrupts and pe_arm_init_debug */
	}
}

void
PE_create_console(void)
{
	if (PE_state.video.v_display)
		PE_initialize_console(&PE_state.video, kPEGraphicsMode);
	else
		PE_initialize_console(&PE_state.video, kPETextMode);
}

uint32_t
PE_i_can_has_debugger(uint32_t *arg)
{
	return (0);
}

void
PE_init_iokit(void)
{
	/* No-op on J2 bring-up */
}

void
PE_init_panicheader(void)
{
	/* No-op on J2 bring-up */
}

int (*PE_poll_input)(unsigned int options, char *c) = NULL;

void
PE_register_timebase_callback(timebase_callback_func callback)
{
	/* No-op on J2 bring-up */
}

void
PE_save_buffer_to_vram(unsigned char *base, unsigned int *size)
{
	/* No-op on J2 bring-up */
}

int
PE_initialize_console(PE_Video *newInfo, int op)
{
	return (0);
}

int
PE_current_console(PE_Video * info)
{
	*info = PE_state.video;
	return (0);
}

void
PE_display_icon(__unused unsigned int flags, __unused const char *name)
{
	/* No-op on J2 bring-up */
}

uint32_t
PE_get_offset_into_panic_region(char *location)
{
	return (0);
}
