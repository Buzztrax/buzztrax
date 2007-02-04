dnl $Id: configure.ac,v 1.90 2007-02-04 21:11:55 ensonic Exp $

dnl
dnl current  : the interface         : MAJOR + MINOR
dnl revision : of the same interface : MICRO
dnl age      : MINOR
dnl
dnl 1: when releasing a changed version : MICRO++
dnl 2: when the interface has changed (adding functions) : MINOR++, MICRO=0
dnl 3: when the interface has changed (removing functions) : MAJOR++, MINOR=0, MICRO=0
dnl

dnl Initialise the autoconf/automake tool
AC_INIT([buzztard], [0.1.0], [buzztard-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR(m4)

BT_MAJOR_VERSION=0
BT_MINOR_VERSION=1
BT_MICRO_VERSION=0
BT_VERSION=$BT_MAJOR_VERSION.$BT_MINOR_VERSION.$BT_MICRO_VERSION
BT_VERSION_INFO=`expr $BT_MAJOR_VERSION + $BT_MINOR_VERSION`:$BT_MICRO_VERSION:$BT_MINOR_VERSION
BT_VERSION_NUMBER=`expr $BT_MAJOR_VERSION \* 10000 + $BT_MINOR_VERSION \* 100 + $BT_MICRO_VERSION`

dnl our libraries and install dirs use major.minor as a version
BT_MAJORMINOR=$BT_MAJOR_VERSION.$BT_MINOR_VERSION
dnl we override it here for release candidates
dnl BT_MAJORMINOR=0.1

dnl update this for each release (year-month-day)
BT_RELEASE_YEAR=2005
BT_RELEASE_DATE=$BT_RELEASE_YEAR-02-10

AC_SUBST(BT_MAJOR_VERSION)
AC_SUBST(BT_MINOR_VERSION)
AC_SUBST(BT_MICRO_VERSION)
AC_SUBST(BT_VERSION)
AC_SUBST(BT_VERSION_INFO)
AC_SUBST(BT_VERSION_NUMBER)
AC_SUBST(BT_MAJORMINOR)
AC_SUBST(BT_RELEASE_YEAR)
AC_SUBST(BT_RELEASE_DATE)

AC_DEFINE_UNQUOTED(PACKAGE_VERSION_NUMBER, ${BT_VERSION_NUMBER}, [version as a number])

VERSION=${BT_VERSION}

dnl Initialise the automake tool
AM_INIT_AUTOMAKE([check-news std-options ])

dnl m4 - everyone who develops should have aclocal installed
AC_PATH_PROG(PROG_ACLOCAL, aclocal, no)
AC_MSG_CHECKING(where to install .m4 files to)
if test "$PROG_ACLOCAL" = "no" ; then
  acdir=${datadir}/aclocal
else
  acdir=`$PROG_ACLOCAL --print-ac-dir`
  if test ! -w "$acdir" ; then
    acdir=${datadir}/aclocal
  fi
fi
AC_MSG_RESULT($acdir)
AC_SUBST(acdir)

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_ISC_POSIX
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL
AM_PROG_CC_STDC
AM_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG

AC_PATH_PROG(xvfb_path, [Xvfb], [no])
if test "x$xvfb_path" != "xno" ; then
  AC_DEFINE_UNQUOTED(XVFB_PATH, "$xvfb_path", [Path of Xvfb executable to be used to run gui unit tests under])
  
  xfont_path=`xset q | grep fonts | sed 's/^\ *//'`
  if test "x$xfont_path" != "x"; then
    AC_DEFINE_UNQUOTED(XFONT_PATH, "$xfont_path", [X font path])
  fi
fi

dnl Initialise gettext
AM_GNU_GETTEXT_VERSION(0.12.1)
AM_GNU_GETTEXT([external])

dnl i18n -- ALL_LINGUAS seems to be not neccesary anymore
dnl ALL_LINGUAS="de"
dnl ALL_LINGUAS="`cat "$srcdir/po/LINGUAS" | grep -v '^#'`"
localedir='${datadir}/locale'
AC_SUBST(localedir)
GETTEXT_PACKAGE="buzztard-$BT_MAJORMINOR"
AC_SUBST(GETTEXT_PACKAGE)
DATADIRNAME="share"
AC_SUBST(DATADIRNAME)

dnl this macro comes from gnome.m4, which we do not want to require
dnl AM_ACLOCAL_INCLUDE(./m4)

dnl enable runtime debugging code
AC_MSG_CHECKING(whether to enable runtime debugging code)
AC_ARG_ENABLE(
  debug,
  AC_HELP_STRING([--enable-debug],[enable runtime debugging code (default=no)]),
  ,
  [enable_debug="no"])
AC_MSG_RESULT($enable_debug)
if test "$enable_debug" = "yes"; then
  AC_DEFINE(USE_DEBUG, [1], [enable runtime debugging code])
  DEBUG_CFLAGS="-O0 -Wall -Werror -g"
  # DEBUG_CFLAGS="-O0 -Wall -Werror -Wno-strict-aliasing -g"
  AM_CONDITIONAL(USE_DEBUG, true)
else
  AC_DEFINE(G_DISABLE_ASSERT, [1], [if no runtime debugging, do not enable g_asserts])
  AC_DEFINE(G_DISABLE_CHECKS, [1], [if no runtime debugging, do not enable g_return_if_fail variants])
  AC_DEFINE(G_DISABLE_CAST_CHECKS, [1], [if no runtime debugging, do not enable dynamic type checks])
  DEBUG_CFLAGS=""
  AM_CONDITIONAL(USE_DEBUG, false)
fi


dnl compile with disable-deprecated switches
AC_MSG_CHECKING([whether to disable deprecated glib/gtk+/gst/etc. features])
AC_ARG_ENABLE(deprecated,
	AC_HELP_STRING([--disable-deprecated],[disable deprecated glib/gtk+/gst/etc. features]),
	set_enable_deprecated="$enableval",[
		if test -f $srcdir/autogen.sh; then
			is_cvs_version=true
			set_enable_deprecated=no
		else
			set_enable_deprecated=yes
		fi
	]
)
if test "$set_enable_deprecated" != "yes"; then
	AC_MSG_RESULT(yes)
	BT_DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGST_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED"
else
	AC_MSG_RESULT(no)
	BT_DISABLE_DEPRECATED=""
fi
AC_SUBST(BT_DISABLE_DEPRECATED)


dnl enable coverage analysis
AC_MSG_CHECKING(whether to enable coverage analysis)
AC_ARG_ENABLE(
  coverage,
  AC_HELP_STRING([--enable-coverage],[enable coverage analysis (default=no)]),
  ,
  [enable_coverage="no"])
AC_MSG_RESULT($enable_coverage)
if test "$enable_coverage" = "yes"; then
  #AC_DEFINE(APP_COVERAGE, [1], [enable coverage analysis])
  AM_CONDITIONAL(USE_COVERAGE, true)
  COVERAGE_CFLAGS="-O0 -g -fprofile-arcs -ftest-coverage"
  COVERAGE_LIBS="-lgcov"
else
  AM_CONDITIONAL(USE_COVERAGE, false)
  COVERAGE_CFLAGS=""
  COVERAGE_LIBS=""
fi

dnl valgrind integration for tests
AC_MSG_CHECKING(where Valgrind is installed on your system)
AC_ARG_WITH([valgrind],
        AC_HELP_STRING([--with-valgrind],
        [where Valgrind is installed on your system (default is no)]),
        [ac_cv_use_valgrind=$withval], [ac_cv_use_valgrind=no])dnl
AC_MSG_RESULT($ac_cv_use_valgrind)
VALGRIND=$ac_cv_use_valgrind
AC_SUBST(VALGRIND)dnl
AM_CONDITIONAL(USE_VALGRIND, test x$VALGRIND != xno)dnl

VALGRIND_CPPFLAGS=
if test $ac_cv_use_valgrind != no; then
   AC_DEFINE(HAVE_VALGRIND, 1, [Define to 1 if valgrind is to be used.])
   if test -f $ac_cv_use_valgrind/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind
   elif test -f $ac_cv_use_valgrind/include/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/include
   elif test -f $ac_cv_use_valgrind/valgrind/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/valgrind
   elif test -f $ac_cv_use_valgrind/include/valgrind/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/include/valgrind
   elif test -f $ac_cv_use_valgrind/valgrind/include/valgrind.h; then
      VALGRIND_CFLAGS=-I$ac_cv_use_valgrind/valgrind/include
   fi
fi
AC_SUBST(VALGRIND_CFLAGS)dnl

dnl define custom path for desktop files
AC_MSG_CHECKING(where to install desktop files)
AC_ARG_WITH(
  desktop_dir,
  AC_HELP_STRING([--with-desktop-dir],[install desktop files to DIR/applications (default=$prefix/share/)]),
  ,
  [with_desktop_dir="$prefix/share/"])
AC_MSG_RESULT($with_desktop_dir)
AC_SUBST(with_desktop_dir)


dnl Checks for libraries.
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= 2.6.0 gobject-2.0  >= 2.6.0 gthread-2.0 >= 2.6.0 gmodule-2.0 >= 2.6.0 \
  gnome-vfs-2.0 >= 2.10 gnome-vfs-module-2.0 >= 2.10 \
  libxml-2.0 >= 2.6.0 \
  gstreamer-0.10 >= 0.10.0 gstreamer-controller-0.10 >= 0.10.0 \
  gstreamer-base-0.10 >= 0.10.0
)

PKG_CHECK_MODULES(GUI_DEPS, \
  gtk+-2.0 >= 2.6.0 \
  libgnomecanvas-2.0
)

PKG_CHECK_MODULES(GCONF_DEPS, gconf-2.0 >= 2.2.0, [
    AM_CONDITIONAL(USE_GCONF, true)
    AC_DEFINE(USE_GCONF, [1], [Define to 1 if we can use GConf])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $GCONF_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $GCONF_DEPS_LIBS"
    AC_PATH_PROG(GCONFTOOL, gconftool-2, [#])
    AM_GCONF_SOURCE_2
  ],[
    AM_CONDITIONAL(USE_GCONF, false)
  ]
)

PKG_CHECK_MODULES(GSTBT_DEPS, libgstbuzztard >= 0.1, [
    AC_DEFINE(USE_GSTBT, [1], [Define to 1 if we can use gst-buzztard])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $GSTBT_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $GSTBT_DEPS_LIBS"
  ]
)

PKG_CHECK_MODULES(GNOME_DEPS, libgnome-2.0 >= 2.6.0, [
    AC_DEFINE(USE_GNOME, [1], [Define to 1 if we can use gnome libraries])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $GNOME_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $GNOME_DEPS_LIBS"
  ],[
    use_gnome=no
  ]
)

PKG_CHECK_MODULES(HILDON_DEPS, hildon-libs libosso, [
  AC_DEFINE(USE_HILDON, [1], [Define to 1 if we can use hildon libraries])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $HILDON_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $HILDON_DEPS_LIBS"
  ],[
    use_hildon=no
  ]
)

glib_prefix="`pkg-config --variable=prefix glib-2.0`"
gtk_prefix="`pkg-config --variable=prefix gtk+-2.0`"
gst_prefix="`pkg-config --variable=prefix gstreamer-0.10`"
gcanvas_prefix="`pkg-config --variable=prefix libgnomecanvas-2.0`"
AC_SUBST(glib_prefix)
AC_SUBST(gtk_prefix)
AC_SUBST(gst_prefix)
AC_SUBST(gcanvas_prefix)

AC_MSG_CHECKING([glib version >= 2.6])
if pkg-config --atleast-version=2.6 glib-2.0; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_GLIB_2_6, [1], [Define to 1 if we can use API new in glib-2.6])
else
  AC_MSG_RESULT(no)
fi
AC_MSG_CHECKING([glib version >= 2.8])
if pkg-config --atleast-version=2.8 glib-2.0; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_GLIB_2_8, [1], [Define to 1 if we can use API new in glib-2.8])
else
  AC_MSG_RESULT(no)
fi
AC_MSG_CHECKING([glib version >= 2.10])
if pkg-config --atleast-version=2.10 glib-2.0; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_GLIB_2_10, [1], [Define to 1 if we can use API new in glib-2.10])
else
  AC_MSG_RESULT(no)
fi

dnl check for pending gtk+ addditions
dnl AC_CHECK_LIB( gtk-x11-2.0, gtk_tree_view_column_set_wrap_header_widget, \
dnl   AC_DEFINE(HAVE_GTK_TREE_VIEW_COLUMN_PATCH, [1], [Define to 1 if we have the pending gtk+ patch applied]) \
dnl )

dnl Add additional libraries
BASE_DEPS_LIBS="$LIBINTL $BASE_DEPS_LIBS"

dnl test for availability of check unittest package (with a relative new function)
dnl @todo: if we configure without --enable-debug most tests wont work
AC_CHECK_HEADER( check.h, \
  AC_CHECK_LIB( check, tcase_set_timeout, \
    AM_CONDITIONAL(BUILD_CHECK_TESTS, true), \
    AM_CONDITIONAL(BUILD_CHECK_TESTS, false) \
  ), \
  AM_CONDITIONAL(BUILD_CHECK_TESTS, false) \
)

dnl Check for gtk-doc.
GTK_DOC_CHECK([1.4])

dnl Check for xsltproc
dnl AC_PATH_PROG(xsltproc, xsltproc, no)
dnl AM_CONDITIONAL(ENABLE_XSLTPROC, test ! x$xsltproc = xno)

dnl Check for scrollkeeper
AC_PATH_PROG(scrollkeeper_config, scrollkeeper-config, no)
if test x$scrollkeeper_config = xno; then
  AC_MSG_WARN([Couldn't find scrollkeeper-config, please install the scrollkeeper package])
dnl  if test x$xsltproc = xno; then
dnl    AC_MSG_WARN([Couldn't find xsltproc, please install the libxml/libxslt package])
dnl  fi
fi
AM_CONDITIONAL(ENABLE_SCROLLKEEPER, test ! x$scrollkeeper_config = xno)
SCROLLKEEPER_BUILD_REQUIRED=0.3.5
AC_SUBST(SCROLLKEEPER_BUILD_REQUIRED)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(X11/Xlocale.h)
AC_CHECK_HEADERS(popt.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AM_C_PROTOTYPES
test "x$U" != "x" && AC_MSG_ERROR(Compiler not ANSI compliant)
AC_C_CONST

dnl Checks for library functions.
dnl glib-2 functions
dnl AC_CHECK_LIB( glib-2.0, g_log_set_default_handler)
dnl libc functions
AC_CHECK_FUNCS( sched_setscheduler)
AC_CHECK_FUNCS( mlockall)

dnl Extra vars
BT_INCLUDEDIR='-I${includedir}/libbtcore -I${includedir}'
dnl -Wl,--as-needed # can be put into CFLAGS to drop all unused libs
BT_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS $BT_DISABLE_DEPRECATED"
BT_LIBDIR='-L${libdir}'
BT_LIBS='-lbtcore -lm $COVERAGE_LIBS'

dnl check for mimetype utilities
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)

dnl Substitutes

AC_SUBST(BT_INCLUDEDIR)
AC_SUBST(BT_CFLAGS)
AC_SUBST(BT_LIBDIR)
AC_SUBST(BT_LIBS)
AC_SUBST(BASE_DEPS_LIBS)
AC_SUBST(BASE_DEPS_CFLAGS)
AC_SUBST(GUI_DEPS_LIBS)
AC_SUBST(GUI_DEPS_CFLAGS)

AC_CONFIG_FILES(Makefile \
  docs/Makefile \
  docs/version.entities \
    docs/help/Makefile \
      docs/help/bt-edit/Makefile \
        docs/help/bt-edit/C/Makefile \
        docs/help/bt-edit/C/bt-edit-C.omf \
    docs/reference/Makefile \
      docs/reference/bt-cmd/Makefile \
      docs/reference/bt-core/Makefile \
      docs/reference/bt-edit/Makefile \
  po/Makefile.in \
  src/Makefile \
    src/lib/Makefile \
    src/lib/buzztard.applications \
    src/lib/buzztard.xml \
    src/lib/buzztard.keys \
      src/lib/core/Makefile \
      src/lib/core/libbtcore.pc \
        src/lib/core/libbtcore/Makefile \
        src/lib/core/libbtcore/version.h \
    src/ui/Makefile \
      src/ui/cmd/Makefile \
      src/ui/edit/Makefile \
      src/ui/edit/bt-edit.desktop.in \
  tests/Makefile \
  tests/bt-cfg.sh \
    tests/lib/Makefile \
      tests/lib/core/Makefile \
    tests/ui/Makefile \
      tests/ui/cmd/Makefile \
      tests/ui/edit/Makefile \
)
AC_OUTPUT
