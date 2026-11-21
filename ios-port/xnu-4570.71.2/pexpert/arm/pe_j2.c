/*
 * J2 (Exynos 3475) Platform Expert adapter for XNU 4570.
 *
 * Provides j2_pe_init_platform() which is called from PE_init_platform.
 * Compiled alongside the original pe_init.c (not replacing it).
 */

#include <pexpert/pexpert.h>
#include <pexpert/boot.h>
#include <pexpert/device_tree.h>

void
j2_pe_init_platform(void *args)
{
	/* J2-specific platform initialization.
	 * Called from PE_init_platform after standard init.
	 * Currently a no-op placeholder for future driver init. */
	(void)args;
}
