# $Id: configure.ac,v 1.12 2004-07-29 15:51:30 ensonic Exp $

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
AC_INIT([buzztard], [0.0.1], [ensonic@users.sourceforge.net])
AC_CONFIG_SRCDIR([ChangeLog])
AC_CONFIG_HEADERS(config.h)

BT_MAJOR_VERSION=0
BT_MINOR_VERSION=0
BT_MICRO_VERSION=1
BT_VERSION=$BT_MAJOR_VERSION.$BT_MINOR_VERSION.$BT_MICRO_VERSION
BT_VERSION_INFO=`expr $BT_MAJOR_VERSION + $BT_MINOR_VERSION`:$BT_MICRO_VERSION:$BT_MINOR_VERSION
BT_VERSION_NUMBER=`expr $BT_MAJOR_VERSION \* 10000 + $BT_MINOR_VERSION \* 100 + $BT_MICRO_VERSION`

dnl our libraries and install dirs use major.minor as a version
BT_MAJORMINOR=$BT_MAJOR_VERSION.$BT_MINOR_VERSION
dnl we override it here for release candidates
#BT_MAJORMINOR=0.8

AC_SUBST(BT_MAJOR_VERSION)
AC_SUBST(BT_MINOR_VERSION)
AC_SUBST(BT_MICRO_VERSION)
AC_SUBST(BT_VERSION)
AC_SUBST(BT_VERSION_INFO)
AC_SUBST(BT_VERSION_NUMBER)
AC_SUBST(BT_MAJORMINOR)

VERSION=${BT_VERSION}

# Initialise the automake tool
AM_INIT_AUTOMAKE(buzztard, $VERSION)

# i18n
dnl ALL_LINGUAS="de en_GB"
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
# we do not use intltool as it merges back translations into xml files
# AC_PROG_INTLTOOL
AM_PROG_CC_STDC

# Initialise gettext
# AM_GNU_GETTEXT([external],[need-ngettext])
# AM_GNU_GETTEXT_VERSION(0.12.1)
# this macro comes from gnome.m4, which we do not want to require
# AM_ACLOCAL_INCLUDE(./m4)

# enable runtime debugging code
AC_MSG_CHECKING(whether to use runtime debugging code)
AC_ARG_ENABLE(
	debug,
	AC_HELP_STRING([--enable-debug],[use runtime debugging code (default=no)]),
	,
	[enable_debug="no"])
AC_MSG_RESULT($enable_debug)
if test "$enable_debug" = "yes"; then
	AC_DEFINE(APP_DEBUG, [1], [enable runtime debugging code])
	AM_CONDITIONAL(DEBUG, true)
else
	AC_DEFINE(G_DISABLE_ASSERT, [1], [if no runtime debugging, don not enable g_asserts])
	AC_DEFINE(G_DISABLE_CHECKS, [1], [if no runtime debugging, don not enable g_return_if_fail variants])
	AC_DEFINE(G_DISABLE_CAST_CHECKS, [1], [if no runtime debugging, don not enable dynamic type checks])
	AM_CONDITIONAL(DEBUG, false)
fi

# enable ansi colored reports
AC_MSG_CHECKING(whether to use ansi-colored reports)
AC_ARG_ENABLE(
	ansi_color,
	AC_HELP_STRING([--enable-ansi-color],[use ansi colored reports (default=yes)]),
	,
	[enable_ansi_color="yes"])
AC_MSG_RESULT($enable_ansi_color)
if test "$enable_ansi_color" = "yes"; then
	AC_DEFINE(USE_ANSI_COLOR, [1], [use ansi colored reports])
fi

# gstreamer-libs not required, while there are only provided from gstreamer-plugins
#	gstreamer-libs-0.8 >= 0.8 gstreamer-interfaces-0.8 >= 0.8 gstreamer-control-0.8 >= 0.8 \

# Checks for libraries.
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= 2.2.0 gthread-2.0 >= 2.2.0 gmodule-2.0 >= 2.2.0 \
  gtk+-2.0 \
	libxml-2.0 >= 2.5.0 libxslt > 1.0.0 \
  gstreamer-0.8 >= 0.8 gstreamer-control-0.8 >= 0.8 \
	)

# Add additional libraries
BASE_DEPS_LIBS="$LIBINTL $BASE_DEPS_LIBS"

# test for availability of check unittest package
AC_CHECK_HEADER( check.h, \
	AC_CHECK_LIB( check, srunner_create, \
		AM_CONDITIONAL(BUILD_CHECK_TESTS, true), \
		AM_CONDITIONAL(BUILD_CHECK_TESTS, false) \
	), \
	AM_CONDITIONAL(BUILD_CHECK_TESTS, false) \
)

# Check for gtk-doc.
AC_ARG_WITH(html-dir, [  --with-html-dir=PATH path to installed docs ])

if test "x$with_html_dir" = "x" ; then
  HTML_DIR='${datadir}/gtk-doc/html'
else
  HTML_DIR=$with_html_dir
fi

AC_SUBST(HTML_DIR)

gtk_doc_min_version=1.0
AC_MSG_CHECKING([gtk-doc version >= $gtk_doc_min_version])
if pkg-config --atleast-version=$gtk_doc_min_version gtk-doc; then
  AC_MSG_RESULT(yes)
  GTKDOC=true
else
  AC_MSG_RESULT(no)
  GTKDOC=false
fi

dnl Let people disable the gtk-doc stuff.
AC_ARG_ENABLE(gtk-doc, [  --enable-gtk-doc  Use gtk-doc to build documentation [default=auto]], enable_gtk_doc="$enableval", enable_gtk_doc=auto)

if test x$enable_gtk_doc = xauto ; then
  if test x$GTKDOC = xtrue ; then
    enable_gtk_doc=yes
  else
    enable_gtk_doc=no 
  fi
fi

AM_CONDITIONAL(ENABLE_GTK_DOC, test x$enable_gtk_doc = xyes)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(X11/Xlocale.h)
AC_CHECK_HEADERS(popt.h)

# Checks for typedefs, structures, and compiler characteristics.
AM_C_PROTOTYPES
test "x$U" != "x" && AC_MSG_ERROR(Compiler not ANSI compliant)
AC_C_CONST

# Checks for library functions.

# Extra vars
BT_CFLAGS=""
BT_LIBDIR='-L${libdir}'
BT_LIBS="-lgitk -lm $LIBS"
BT_INCLUDEDIR='-I${includedir}/libbtcore -I${includedir}'

# Substitutes

AC_SUBST(BT_CFLAGS)
AC_SUBST(BT_LIBDIR)
AC_SUBST(BT_LIBS)
AC_SUBST(BT_INCLUDEDIR)
AC_SUBST(BASE_DEPS_LIBS)
AC_SUBST(BASE_DEPS_CFLAGS)

AC_CONFIG_FILES(Makefile \
  docs/Makefile
    docs/version.entities \
    docs/reference/Makefile \
  src/Makefile \
		src/lib/Makefile \
			src/lib/core/Makefile \
			src/lib/core/libbtcore/version.h \
		src/ui/Makefile \
			src/ui/cmd/Makefile \
			src/ui/edit/Makefile \
  tests/Makefile \
		tests/lib/Makefile \
			tests/lib/core/Makefile \
		tests/ui/Makefile \
			tests/ui/cmd/Makefile \
)
AC_OUTPUT
