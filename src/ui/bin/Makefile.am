## Makefile.am for src/ui/bin

plugin_LTLIBRARIES = libbuzztardbin.la

# sources used to compile this plug-in
libbuzztardbin_la_SOURCES = bt-bin.c

# flags used to compile this plugin
libbuzztardbin_la_CFLAGS = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS)
libbuzztardbin_la_LIBADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
    $(top_builddir)/src/lib/ic/libbuzztard-ic.la \
    $(BASE_DEPS_LIBS)
libbuzztardbin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libbuzztardbin_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = bt-bin.h

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
	splint +posixlib -weak -I$(top_builddir) $(AM_CPPFLAGS) $(libbtcmd_la_SOURCES)

help::
	@echo "make todo                    -- generate a list of TODO items in the form of gcc error/warnings"
	@echo "make stats                   -- run source code stats"
	@echo "make splint                  -- run splint source code checks"