import sys

p = sys.argv[1]
s = open(p).read()

needle = "\tStartIOKit(PE_state.deviceTreeHead, PE_state.bootArgs, (void *) 0, (void *) 0);\n}\n"
assert needle in s, "needle not found in pe_init.c"
assert "j2_pe_init_platform" not in s, "patch already applied"

s = s.replace(needle, "\tStartIOKit(PE_state.deviceTreeHead, PE_state.bootArgs, (void *) 0, (void *) 0);\n\n\t/* J2 platform adapter (incremental, original PE preserved) */\n\tj2_pe_init_platform((void *)0);\n}\n", 1)

hdr = "#include <pexpert/pexpert.h>\n"
assert hdr in s, "header include not found"
s = s.replace(hdr, hdr + "\nextern void j2_pe_init_platform(void *);\n", 1)

# Append function definition at end of file
s += '''

void
j2_pe_init_platform(void *args)
{
	(void)args;
	/* J2 (Exynos 3475) platform adapter - incremental integration */
}
'''

open(p, "w").write(s)
print("patched pe_init.c (J2 adapter inserted + defined)")
PYEOF
