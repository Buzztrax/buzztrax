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
	main-window.c main-window.h main-window-methods.h \
	main-menu.c main-menu.h main-menu-methods.h \
	main-toolbar.c main-toolbar.h main-toolbar-methods.h \
	main-pages.c main-pages.h main-pages-methods.h \
	main-page-info.c main-page-info.h main-page-info-methods.h \
	main-statusbar.c main-statusbar.h main-statusbar-methods.h
	
INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

