
# dependency files
TOOLS_DEPS := tools/mkfs.d

# all generated files
TOOLS_CLEAN := tools/mkfs tools/mkfs.o $(TOOLS_DEPS)

# flags
TOOLS_CPPFLAGS := -iquote include

# mkfs.c uses readdir_r, which newer glibc marks deprecated; with -Werror
# that warning would stop the build, so allow it for the host tools only
TOOLS_CFLAGS := -Wno-deprecated-declarations

# mkfs
tools/mkfs: tools/mkfs.o
	$(CC) $(LDFLAGS) $< -o $@

# build object files from c files
tools/%.o: tools/%.c
	$(CC) -c $(CPPFLAGS) $(TOOLS_CPPFLAGS) $(CFLAGS) $(TOOLS_CFLAGS) -o $@ $<

# build dependency files form c files
tools/%.d: tools/%.c
	$(CC) $(CPPFLAGS) $(TOOLS_CPPFLAGS) $(CFLAGS) $(TOOLS_CFLAGS) \
	  -M -MG $< -MF $@ -MT $@ -MT $(<:.c=.o)

