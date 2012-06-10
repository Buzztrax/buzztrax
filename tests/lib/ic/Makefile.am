noinst_LTLIBRARIES = libbtic-check.la
libbtic_check_la_LIBADD = $(BASE_DEPS_LIBS)
libbtic_check_la_SOURCES = m-bt-ic.h \
	s-ic.c e-ic.c t-ic.c \
	s-registry.c e-registry.c

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/ic \
	-I$(top_srcdir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS)  \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)

include $(top_srcdir)/common.mak
