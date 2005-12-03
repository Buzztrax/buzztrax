# $Id: configure.ac,v 1.65 2005-12-03 16:24:21 ensonic Exp $

#
# current  : the interface         : MAJOR + MINOR
# revision : of the same interface : MICRO
# age      : MINOR
#
# 1: when releasing a changed version : MICRO++
# 2: when the interface has changed (adding functions) : MINOR++, MICRO=0
# 3: when the interface has changed (removing functions) : MAJOR++, MINOR=0, MICRO=0
#

# Initialise the autoconf/automake tool
AC_INIT([buzztard], [0.0.1], [buzztard-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR(m4)

BT_MAJOR_VERSION=0
BT_MINOR_VERSION=0
BT_MICRO_VERSION=1
BT_VERSION=$BT_MAJOR_VERSION.$BT_MINOR_VERSION.$BT_MICRO_VERSION
BT_VERSION_INFO=`expr $BT_MAJOR_VERSION + $BT_MINOR_VERSION`:$BT_MICRO_VERSION:$BT_MINOR_VERSION
BT_VERSION_NUMBER=`expr $BT_MAJOR_VERSION \* 10000 + $BT_MINOR_VERSION \* 100 + $BT_MICRO_VERSION`

dnl our libraries and install dirs use major.minor as a version
BT_MAJORMINOR=$BT_MAJOR_VERSION.$BT_MINOR_VERSION
dnl we override it here for release candidates
#BT_MAJORMINOR=0.1

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

VERSION=${BT_VERSION}

# Initialise the automake tool
AM_INIT_AUTOMAKE([check-news std-options ])

# i18n
ALL_LINGUAS="de"
localedir='${datadir}/locale'
AC_SUBST(localedir)

# m4 - everyone who develops should have aclocal installed
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

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_ISC_POSIX
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL
AM_PROG_CC_STDC

AC_PATH_PROG(xvfb_path, [Xvfb], [no])
if test "x$xvfb_path" != "xno" ; then
  AC_DEFINE_UNQUOTED(XVFB_PATH, "$xvfb_path", [Path of Xvfb executable to be used to run gui unit tests under])
fi

# Initialise gettext
AM_GNU_GETTEXT([external],[need-ngettext])
AM_GNU_GETTEXT_VERSION(0.12.1)
# this macro comes from gnome.m4, which we do not want to require
# AM_ACLOCAL_INCLUDE(./m4)

# enable runtime debugging code
AC_MSG_CHECKING(whether to enable runtime debugging code)
AC_ARG_ENABLE(
  debug,
  AC_HELP_STRING([--enable-debug],[enable runtime debugging code (default=no)]),
  ,
  [enable_debug="no"])
AC_MSG_RESULT($enable_debug)
if test "$enable_debug" = "yes"; then
  AC_DEFINE(APP_DEBUG, [1], [enable runtime debugging code])
  DEBUG_CFLAGS="-O0 -Wall -Werror -g"
else
  AC_DEFINE(G_DISABLE_ASSERT, [1], [if no runtime debugging, do not enable g_asserts])
  AC_DEFINE(G_DISABLE_CHECKS, [1], [if no runtime debugging, do not enable g_return_if_fail variants])
  AC_DEFINE(G_DISABLE_CAST_CHECKS, [1], [if no runtime debugging, do not enable dynamic type checks])
  DEBUG_CFLAGS=""
fi


# compile with disable-deprecated switches
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
	BT_DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGST_DISABLE_DEPRECATED"
else
	AC_MSG_RESULT(no)
	BT_DISABLE_DEPRECATED=""
fi
AC_SUBST(BT_DISABLE_DEPRECATED)


# enable coverage analysis
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
else
  AM_CONDITIONAL(USE_COVERAGE, false)
  COVERAGE_CFLAGS=""
fi

# valgrind integration for tests
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

# define custom path for desktop files
AC_MSG_CHECKING(where to install desktop files)
AC_ARG_WITH(
  desktop_dir,
  AC_HELP_STRING([--with-desktop-dir],[install desktop files to DIR/applications (default=$prefix/share/)]),
  ,
  [with_desktop_dir="$prefix/share/"])
AC_MSG_RESULT($with_desktop_dir)
AC_SUBST(with_desktop_dir)


# Checks for libraries.
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= 2.6.0 gthread-2.0 >= 2.6.0 gmodule-2.0 >= 2.6.0 \
  gnome-vfs-2.0 >= 2.10 gnome-vfs-module-2.0 >= 2.10 \
  libxml-2.0 >= 2.6.0 \
  gstreamer-0.10 >= 0.9.7 gstreamer-controller-0.10 >= 0.9.7 \
)

PKG_CHECK_MODULES(GUI_DEPS, \
  gtk+-2.0 >= 2.6.0 \
  libgnomecanvas-2.0
)

glib_prefix="`pkg-config --variable=prefix glib-2.0`"
gtk_prefix="`pkg-config --variable=prefix gtk+-2.0`"
gst_prefix="`pkg-config --variable=prefix gstreamer-0.10`"
gcanvas_prefix="`pkg-config --variable=prefix libgnomecanvas-2.0`"
AC_SUBST(glib_prefix)
AC_SUBST(gtk_prefix)
AC_SUBST(gst_prefix)
AC_SUBST(gcanvas_prefix)

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

PKG_CHECK_MODULES(GSTBT_DEPS, libgstbuzztard >= 0.0.1, [
    AC_DEFINE(USE_GSTBT, [1], [Define to 1 if we can use gst-buzztard])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $GSTBT_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $GSTBT_DEPS_LIBS"
  ]
)

#PKG_CHECK_MODULES(GTKGRID_DEPS, gtkgrid, [
#  AM_CONDITIONAL(USE_GTKGRID, true)
#    AC_DEFINE(USE_GTKGRID, [1], [Define to 1 if we can use GtkGrid])
#    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $GTKGRID_DEPS_CFLAGS"
#    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $GTKGRID_DEPS_LIBS"
# ],[
#    AM_CONDITIONAL(USE_GTKGRID,false)
# ]
#)

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

# Add additional libraries
BASE_DEPS_LIBS="$LIBINTL $BASE_DEPS_LIBS"

# test for availability of check unittest package (with a relative new function)
# @todo: if we configure without --enable-debug most tests wont work
AC_CHECK_HEADER( check.h, \
  AC_CHECK_LIB( check, tcase_set_timeout, \
    AM_CONDITIONAL(BUILD_CHECK_TESTS, true), \
    AM_CONDITIONAL(BUILD_CHECK_TESTS, false) \
  ), \
  AM_CONDITIONAL(BUILD_CHECK_TESTS, false) \
)

# Check for gtk-doc.
GTK_DOC_CHECK(1.4)

# Check for xsltproc
#AC_PATH_PROG(xsltproc, xsltproc, no)
#AM_CONDITIONAL(ENABLE_XSLTPROC, test ! x$xsltproc = xno)

# Check for scrollkeeper
AC_PATH_PROG(scrollkeeper_config, scrollkeeper-config, no)
if test x$scrollkeeper_config = xno; then
  AC_MSG_WARN([Couldn't find scrollkeeper-config, please install the scrollkeeper package])
#  if test x$xsltproc = xno; then
#    AC_MSG_WARN([Couldn't find xsltproc, please install the libxml/libxslt package])
#  fi
fi
AM_CONDITIONAL(ENABLE_SCROLLKEEPER, test ! x$scrollkeeper_config = xno)
SCROLLKEEPER_BUILD_REQUIRED=0.3.5
AC_SUBST(SCROLLKEEPER_BUILD_REQUIRED)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(X11/Xlocale.h)
AC_CHECK_HEADERS(popt.h)

# Checks for typedefs, structures, and compiler characteristics.
AM_C_PROTOTYPES
test "x$U" != "x" && AC_MSG_ERROR(Compiler not ANSI compliant)
AC_C_CONST

# Checks for library functions.
# glib-2 functions
# AC_CHECK_LIB( libglib-2.0, g_log_set_default_handler)

# Extra vars
BT_INCLUDEDIR='-I${includedir}/libbtcore -I${includedir}'
# -Wl,--as-needed # can be put into CFLAGS to drop all unused libs
BT_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS $BT_DISABLE_DEPRECATED"
BT_LIBDIR='-L${libdir}'
BT_LIBS='-lbtcore -lm'

# check for mimetype utilities
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)

# Substitutes

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
        docs/help/bt-edit/C/bt-edit.xml \
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
    tests/lib/Makefile \
      tests/lib/core/Makefile \
    tests/ui/Makefile \
      tests/ui/cmd/Makefile \
      tests/ui/edit/Makefile \
)
AC_OUTPUT
