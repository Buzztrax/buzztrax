## Makefile.am for src/ui/cmd

bin_PROGRAMS=bt-cmd

bt_cmd_LDADD = $(BASE_DEPS_LIBS) \
	$(top_builddir)/src/lib/core/libbtcore.la \
	libbtcmd.la
bt_cmd_SOURCES= \
	bt-cmd.c bt-cmd.h

noinst_LTLIBRARIES      = libbtcmd.la
libbtcmd_la_SOURCES   =	\
	cmd-application.c cmd-application.h cmd-application-methods.h

INCLUDES = \
	-I@srcdir@ -I$(top_srcdir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

.PHONY: help todo stats splint

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@idea .*" {} \; | sed "s/@idea/warning:/"

stats:: tags
	@echo "files by size:"
	@find . \( -name "*.c" -o -name "*.h" \) -printf "%s %p\n" | sort -rn
	@echo
	@echo "files by tags:"
	@for file in *.c; do size=`grep $${file} tags | wc -l`;echo $${size} $${file}; done | sort -rn

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbtcmd_la_SOURCES)

help::
	@echo "make todo                    -- generate a list of TODO items in the form of gcc error/warnings"
	@echo "make stats                   -- run source code stats"
	@echo "make splint                  -- run splint source code checks"
