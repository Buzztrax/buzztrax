## Makefile.am for src/ui/edit

bin_PROGRAMS=bt-edit

bt_edit_LDADD = $(BASE_DEPS_LIBS) \
	$(top_builddir)/src/lib/core/libbtcore.la \
	libbtedit.la
bt_edit_SOURCES= \
	bt-edit.c bt-edit.h 

noinst_LTLIBRARIES      = libbtedit.la
libbtedit_la_SOURCES   = \
	edit-application.c edit-application.h edit-application-methods.h \
	main-window.c main-window.h main-window-methods.h
	
INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

