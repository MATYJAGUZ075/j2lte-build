import sys

p = sys.argv[1]
s = open(p).read()
needle = "\tpe_init_debug();\n"
assert needle in s, "needle not found"
s = s.replace(needle, needle + "\n\t/* J2 platform adapter */\n\tj2_pe_init_platform(args);\n", 1)
s = s.replace("#include <libkern/section_keywords.h>\n",
              "#include <libkern/section_keywords.h>\n\nextern void j2_pe_init_platform(void *);\n", 1)
open(p, "w").write(s)
print("patched pe_init.c")
