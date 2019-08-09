dnl Initialise autoconf/automake/libtool
AC_PREREQ([2.69])
AC_INIT([buzztrax],[0.11.0],[buzztrax-devel@buzztrax.org])
AC_CANONICAL_TARGET([])
# we can't do -Werror, because
# - we use gnu make extensions
# - we override uninstallcheck var
AM_INIT_AUTOMAKE([-Wall 1.14 check-news std-options foreign tar-ustar subdir-objects])
AC_CONFIG_SRCDIR([README.md])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR(m4)
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_PREREQ([2.2])
LT_INIT

# Support silent build rules, requires at least automake-1.11. Disable by
# either passing --disable-silent-rules to configure or passing V=1 to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


dnl dependencies
REQ_GLIB=2.44.0
REQ_GTK=3.4
dnl TODO(ensonic): probably need to bump to 1.6.0 for working gstcontext
REQ_GST=1.2.0


dnl
dnl current  : the interface         : MAJOR + MINOR
dnl revision : of the same interface : MICRO
dnl age      : MINOR
dnl
dnl 1: when releasing a changed version : MICRO++
dnl 2: when the interface has changed (adding functions) : MINOR++, MICRO=0
dnl 3: when the interface has changed (removing functions) : MAJOR++, MINOR=0, MICRO=0
dnl
BT_MAJOR_VERSION=1
BT_MINOR_VERSION=1
BT_MICRO_VERSION=0
BT_VERSION=$BT_MAJOR_VERSION.$BT_MINOR_VERSION.$BT_MICRO_VERSION
BT_VERSION_INFO=`expr $BT_MAJOR_VERSION + $BT_MINOR_VERSION`:$BT_MICRO_VERSION:$BT_MINOR_VERSION

BT_MAJORMINOR=$BT_MAJOR_VERSION.$BT_MINOR_VERSION
AC_DEFINE_UNQUOTED(BT_MAJORMINOR, "$BT_MAJORMINOR", [buzztrax series])
AC_SUBST(BT_MAJORMINOR)

dnl that is needed to figure the directory name, where the plugin goes to
GST_MAJORMINOR=1.0
AC_DEFINE_UNQUOTED(GST_MAJORMINOR, "$GST_MAJORMINOR", [gstreamer series])
AC_SUBST(GST_MAJORMINOR)

dnl release year and date
DATE_STAMP=`head -n1 "${srcdir}/NEWS" | sed -e 's/^.*(\([[^\)]]*\)).*$/\1/'`
if test "$DATE_STAMP" == "XX.XXX.XXXX"; then
  BT_RELEASE_YEAR=`date +%Y`
  BT_RELEASE_DATE=`date +%Y-%m-%d`
else
  IFS="." read -r d m y <<< "$DATE_STAMP"
  BT_RELEASE_YEAR="$y"
  BT_RELEASE_DATE=`date -d "%d %m %y" +%Y-%m-%d`
fi

AC_SUBST(BT_MAJOR_VERSION)
AC_SUBST(BT_MINOR_VERSION)
AC_SUBST(BT_MICRO_VERSION)
AC_SUBST(BT_VERSION)
AC_SUBST(BT_VERSION_INFO)
AC_SUBST(BT_RELEASE_YEAR)
AC_SUBST(BT_RELEASE_DATE)

AC_DEFINE_UNQUOTED(BT_VERSION, "$BT_VERSION", [library version as string])
AC_DEFINE_UNQUOTED(PACKAGE_VERSION_NUMBER, 900, [version as a number])
AC_DEFINE_UNQUOTED(BT_RELEASE_YEAR, $BT_RELEASE_YEAR, [release year])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PROG_SED
AM_PROG_AS
AM_PROG_CC_C_O
IT_PROG_INTLTOOL
PKG_PROG_PKG_CONFIG

dnl Initialise gettext
AM_GNU_GETTEXT_VERSION(0.12.1)
AM_GNU_GETTEXT([external])
localedir='${datadir}/locale'
AC_SUBST(localedir)
GETTEXT_PACKAGE="buzztrax-$PACKAGE_VERSION"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Package Name for Gettext])

dnl check if we build from vcs or tarball
AC_MSG_CHECKING(if this is a development or release build)
if test -f "${srcdir}/autogen.sh"; then
  dev_build=yes
  rel_build=no
  AC_DEFINE(DEV_BUILD, [1], [defined when building from version control system])
  AC_MSG_RESULT(development)
else
  dev_build=no
  rel_build=yes
  AC_MSG_RESULT(release)
fi

dnl enable runtime debugging code
AC_MSG_CHECKING(whether to enable runtime debugging code)
AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug],[enable runtime debugging code (default=no)]),
  ,
  [enable_debug=no])
AC_MSG_RESULT($enable_debug)
if test "x$enable_debug" = "xyes"; then
  AC_DEFINE(USE_DEBUG, [1], [enable runtime debugging code])
  DEBUG_CFLAGS="-O0 -Wall -Werror -g"
  DEBUG_LDFLAGS="-Wl,--warn-common"
else
  AC_DEFINE(G_DISABLE_ASSERT, [1], [if no runtime debugging, do not enable g_asserts])
  AC_DEFINE(G_DISABLE_CHECKS, [1], [if no runtime debugging, do not enable g_return_if_fail variants])
  AC_DEFINE(G_DISABLE_CAST_CHECKS, [1], [if no runtime debugging, do not enable dynamic type checks])
  DEBUG_CFLAGS=""
  DEBUG_LDFLAGS=""
fi
AM_CONDITIONAL(USE_DEBUG, test "x$enable_debug" = "xyes")

dnl compile with disable-deprecated switches
AC_MSG_CHECKING([whether to disable deprecated glib/gtk+/gst/etc. features])
AC_ARG_ENABLE([deprecated],
	AS_HELP_STRING([--disable-deprecated],[disable deprecated glib/gtk+/gst/etc. features]),
	,
	[enable_deprecated=$rel_build]
)
AC_MSG_RESULT($enable_deprecated)
if test "x$enable_deprecated" = "xno"; then
	# ATK_DISABLE_DEPRECATED
	BT_DISABLE_DEPRECATED="-DG_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGST_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED"
else
	BT_DISABLE_DEPRECATED="-DGLIB_DISABLE_DEPRECATION_WARNINGS -DGDK_DISABLE_DEPRECATION_WARNINGS -DCLUTTER_DISABLE_DEPRECATION_WARNINGS"
fi
AC_SUBST(BT_DISABLE_DEPRECATED)


dnl enable coverage analysis
AC_MSG_CHECKING(whether to enable coverage analysis)
AC_ARG_ENABLE([coverage],
  AS_HELP_STRING([--enable-coverage],[enable coverage analysis (default=no)]),
  ,
  [enable_coverage=no])
AC_MSG_RESULT($enable_coverage)
COVERAGE_CFLAGS=""
COVERAGE_LIBS=""
if test "x$enable_coverage" = "xyes"; then
  bcov_path="no"
  lcov_path="no"
  dnl bcov isn't ready yet
  AC_PATH_PROG(bcov_path, [bcov], [no])
  if test "x$bcov_path" != "xno" ; then
    coverage_tool="bcov"
  else
    AC_PATH_PROG(lcov_path, [lcov], [no])
    if test "x$lcov_path" != "xno" ; then
      LCOV_VERSION=`lcov --version|head -n 1| $SED 's/^.* version //'`
      LCOV_MAJOR=`echo $LCOV_VERSION | cut -d. -f1 | $SED s/[a-zA-Z\-].*//g`
      LCOV_MINOR=`echo $LCOV_VERSION | cut -d. -f2 | $SED s/[a-zA-Z\-].*//g`
      if [[ "$LCOV_MAJOR" -lt "1" ]]; then
        AC_MSG_WARN([lcov >= 1.6 is required, you have $LCOV_VERSION])
        lcov_path="no"
      elif [[ "$LCOV_MAJOR" -eq "1" ]] && \
           [[ "$LCOV_MINOR" -lt "6" ]]; then
        AC_MSG_WARN([lcov >= 1.6 is required, you have $LCOV_VERSION])
        lcov_path="no"
      fi
      if test "x$lcov_path" != "xno" ; then
        coverage_tool="gcov + lcov"
        COVERAGE_CFLAGS="-O0 -g -fprofile-arcs -ftest-coverage"
        COVERAGE_LIBS="-lgcov"
      fi
    else
      coverage_tool="gcov"
      COVERAGE_CFLAGS="-O0 -g -fprofile-arcs -ftest-coverage"
      COVERAGE_LIBS="-lgcov"
    fi
  fi
fi
AM_CONDITIONAL(USE_BCOV, test "x$bcov_path" != "xno")
AM_CONDITIONAL(USE_LCOV, test "x$lcov_path" != "xno")


dnl valgrind integration for tests
AC_PATH_PROG(valgrind_path, [valgrind], [no])
if test "x$valgrind_path" != "xno"; then
  valgrind_path=`dirname $valgrind_path`
  VALGRIND=`dirname $valgrind_path`
  have_valgrind="yes"
else
  VALGRIND="no"
  have_valgrind="no"
fi
AC_SUBST(VALGRIND)
AM_CONDITIONAL(USE_VALGRIND, test "x$VALGRIND" != "xno")


dnl xvfb for hidden ui tests
AC_PATH_PROG(xvfb_path, [Xvfb], [no])
if test "x$xvfb_path" != "xno" ; then
  have_xvfb="yes"
  AC_DEFINE_UNQUOTED(XVFB_PATH, "$xvfb_path", [Path of Xvfb executable to be used to run gui unit tests under])

  XFONT_PATH=`xset q | grep fonts | sed 's/^\ *//'`
  if test "x$XFONT_PATH" != "x"; then
    AC_DEFINE_UNQUOTED(XFONT_PATH, "$XFONT_PATH", [X font path])
  fi
else
  have_xvfb="no"
fi
AC_SUBST(xvfb_path)
AC_SUBST(XFONT_PATH)


dnl define custom path for desktop files
AC_MSG_CHECKING(where to install desktop files)
AC_ARG_WITH(
  desktop_dir,
  AS_HELP_STRING([--with-desktop-dir],[install desktop files to DIR/applications (default=$prefix/share/)]),
  ,
  [with_desktop_dir="$prefix/share/"])
AC_MSG_RESULT($with_desktop_dir)
AC_SUBST(with_desktop_dir)


dnl check if we have 32-bit headers installed
ac_cppflags_save="$CPPFLAGS"
CPPFLAGS="-m32"
AC_CHECK_HEADERS([features.h],
  [
    have_x32_h=yes
  ],
  [
    have_x32_h=no
  ]
)
CPPFLAGS="$ac_cppflags_save"

dnl check target cpu architecture
BMLHOST_CFLAGS=""
BMLHOST_CCASFLAGS=""
case "x${target_cpu}" in
  xi?86 | k?)
    HAVE_X86=yes
    CALLING_MODE=direct
    ;;
  xx86_64|xamd64)
    dnl switch 'xyes' to 'yes' to build it for 32bit using the ipc wrapper
    if test "$have_x32_h" = "xno"; then
      HAVE_X86=yes
      BMLHOST_CFLAGS="-m32"
      BMLHOST_CCASFLAGS="-m32"
      CALLING_MODE=ipc
    else
      HAVE_X86=no
      CALLING_MODE=void
    fi
    ;;
  *)
    HAVE_X86=no
    CALLING_MODE=void
    ;;
esac

AC_MSG_CHECKING(whether to include the emulation for buzzmachine dlls)
AC_ARG_ENABLE(
	dllwrapper,
	AS_HELP_STRING([--enable-dllwrapper],[enable emulation for buzzmachine dlls (default=auto)]),
	,
	[enable_dllwrapper="auto"])
if test "$enable_dllwrapper" != "no"; then
  if test "x$HAVE_X86" = "xno"; then
    enable_dllwrapper="no"
  else
    enable_dllwrapper="yes"
  fi
fi
if test "$enable_dllwrapper" == "no"; then
  CALLING_MODE=void
fi
AC_MSG_RESULT($enable_dllwrapper)
if test "$enable_dllwrapper" = "yes"; then
  AC_DEFINE(USE_DLLWRAPPER, 1, [Defined if emulation for buzzmachine dlls is enabled])
fi
if test "$CALLING_MODE" = "direct"; then
  AC_DEFINE(USE_DLLWRAPPER_DIRECT, 1, [Defined if emulation for buzzmachine dlls is using direct calls])
fi
if test "$CALLING_MODE" = "ipc"; then
  AC_DEFINE(USE_DLLWRAPPER_IPC, 1, [Defined if emulation for buzzmachine dlls is using ipc])
fi
AM_CONDITIONAL(USE_DLLWRAPPER, test x$enable_dllwrapper = xyes)
AM_CONDITIONAL(USE_DLLWRAPPER_DIRECT, test x$CALLING_MODE = xdirect)
AM_CONDITIONAL(USE_DLLWRAPPER_IPC, test x$CALLING_MODE = xipc)


dnl Checks for libraries.

dnl mandarory deps
PKG_CHECK_MODULES(BASE_DEPS, \
  glib-2.0 >= $REQ_GLIB gobject-2.0  >= $REQ_GLIB gthread-2.0 >= $REQ_GLIB \
  gmodule-2.0 >= $REQ_GLIB gio-2.0 >= $REQ_GLIB \
  libxml-2.0 >= 2.6.0 \
  gstreamer-1.0 >= $REQ_GST gstreamer-controller-1.0 >= $REQ_GST \
  gstreamer-base-1.0 >= $REQ_GST \
  gstreamer-plugins-base-1.0 >= $REQ_GST \
  gstreamer-audio-1.0 >= $REQ_GST \
  gstreamer-fft-1.0 >= $REQ_GST \
)

PKG_CHECK_MODULES(GUI_DEPS, \
  clutter-1.0 clutter-gtk-1.0 gtk+-3.0 >= $REQ_GTK \
)

PKG_CHECK_EXISTS(gtk+-3.0 >= 3.14, have_gtk_3_14="yes", have_gtk_3_14="no")
AM_CONDITIONAL(HAVE_GTK_3_14, test "x$have_gtk_3_14" == "xyes")
PKG_CHECK_EXISTS(gtk+-3.0 >= 3.19.6, have_gtk_3_19_6="yes", have_gtk_3_19_6="no")
AM_CONDITIONAL(HAVE_GTK_3_19_6, test "x$have_gtk_3_19_6" == "xyes")

PKG_CHECK_MODULES(ALSA_DEPS, alsa, [
    AC_DEFINE(USE_ALSA, [1], [Define to 1 if we can use alsa libraries])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $ALSA_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $ALSA_DEPS_LIBS"
    use_alsa=yes
    ALSA_DOC_TYPES=
    ALSA_DOC_SECTIONS=
  ],[
    use_alsa=no
    ALSA_DOC_TYPES=%
    ALSA_DOC_SECTIONS=#
  ]
)
AM_CONDITIONAL(USE_ALSA, test "x$use_alsa" == "xyes")
AC_SUBST(ALSA_DOC_TYPES)
AC_SUBST(ALSA_DOC_SECTIONS)

PKG_CHECK_MODULES(GUDEV_DEPS, gudev-1.0, [
    AC_DEFINE(USE_GUDEV, [1], [Define to 1 if we can use gudev libraries])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $GUDEV_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $GUDEV_DEPS_LIBS"
    use_gudev=yes
    GUDEV_DOC_TYPES=
    GUDEV_DOC_SECTIONS=
  ],[
    use_gudev=no
    GUDEV_DOC_TYPES=%
    GUDEV_DOC_SECTIONS=#
  ]
)
AM_CONDITIONAL(USE_GUDEV, test "x$use_gudev" == "xyes")
AC_SUBST(GUDEV_DOC_TYPES)
AC_SUBST(GUDEV_DOC_SECTIONS)

PKG_CHECK_MODULES(GSF_DEPS, libgsf-1, [
    AC_DEFINE(USE_GSF, [1], [Define to 1 if we can use gsf libraries])
    BASE_DEPS_CFLAGS="$BASE_DEPS_CFLAGS $GSF_DEPS_CFLAGS"
    BASE_DEPS_LIBS="$BASE_DEPS_LIBS $GSF_DEPS_LIBS"
    use_gsf=yes
  ],[
    use_gsf=no
  ]
)
AM_CONDITIONAL(USE_GSF, test "x$use_gsf" == "xyes")

GLIB_GSETTINGS

# GObject Introspection
AC_ARG_ENABLE([introspection],
  AS_HELP_STRING([--enable-introspection],
     [enable introspection for this build (default=auto)]),
     ,
     [enable_introspection=auto])

case "x${enable_introspection}" in
  xyes|xauto)
    PKG_CHECK_MODULES(GOBJECT_INTROSPECTION, [gobject-introspection-1.0],[
        G_IR_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
        AC_SUBST(G_IR_SCANNER)
        G_IR_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`
        AC_SUBST(G_IR_COMPILER)
        G_IR_GENERATE=`$PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0`
        AC_SUBST(G_IR_GENERATE)
        GIRDIR=`$PKG_CONFIG --variable=girdir gobject-introspection-1.0`
        AC_SUBST(GIRDIR)
        TYPELIBDIR=`$PKG_CONFIG --variable=typelibdir gobject-introspection-1.0`
        AC_SUBST(TYPELIBDIR)
        enable_gir="yes"
      ],[
        enable_gir="no"
      ]
    )
    ;;
  *)
    enable_gir="no"
    ;;
esac
AM_CONDITIONAL(USE_GIR, test "x$enable_gir" == "xyes")

GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
GTK_PREFIX="`$PKG_CONFIG --variable=prefix gtk+-3.0`"
GST_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-1.0`"
AC_SUBST(GLIB_PREFIX)
AC_SUBST(GTK_PREFIX)
AC_SUBST(GST_PREFIX)

dnl check for pending gtk+ addditions
dnl ac_libs_save="$LIBS"
dnl AC_CHECK_LIB( gtk-x11-2.0, gtk_tree_view_column_set_wrap_header_widget, \
dnl   AC_DEFINE(HAVE_GTK_TREE_VIEW_COLUMN_PATCH, [1], [Define to 1 if we have the pending gtk+ patch applied]) \
dnl )
dnl LIBS="$ac_libs_save"

dnl check for gstreamer devel additions
#ac_cflags_save="$CFLAGS"
#ac_cppflags_save="$CPPFLAGS"
#ac_libs_save="$LIBS"
#CFLAGS="$CFLAGS $BASE_DEPS_CFLAGS"
#CPPFLAGS="$CPPFLAGS $BASE_DEPS_CFLAGS"
#LIBS="$LIBS $BASE_DEPS_LIBS"
#AC_CHECK_HEADERS(gst/gstpreset.h)
#AC_CHECK_FUNCS(gst_element_factory_can_sink_any_caps)
#LIBS="$ac_libs_save"
#CFLAGS="$ac_cflags_save"
#CPPFLAGS="$ac_cppflags_save"
#LIBS="$ac_libs_save"

# TODO(ensonic: remove when we require gst-1.6
ac_libs_save="$LIBS"
AC_CHECK_LIB( gstcontroller-1.0, gst_direct_control_binding_new_absolute, \
   AC_DEFINE(HAVE_GST_CONTROL_BINDING_ABS, [1], [Define to 1 if we have abs direct control binding]) \
)
AM_CONDITIONAL(BUILD_GST_CONTROL_BINDING_ABS, test "x$ac_cv_lib_gstcontroller_1_0_gst_direct_control_binding_new_absolute" == "xno")
LIBS="$ac_libs_save"

dnl Add additional libraries
BASE_DEPS_LIBS="$LIBINTL $BASE_DEPS_LIBS"

dnl test for availability of check unittest package (with a relative new function)
dnl @todo: if we configure without --enable-debug most tests wont work
PKG_CHECK_MODULES(CHECK, check >= 0.9.2,
  [
    have_check=yes
  ],[
    have_check=no
  ]
)
AM_CONDITIONAL(BUILD_CHECK_TESTS, test "x$have_check" == "xyes")

dnl existing plugins are here (this might be outside of $prefix)
GST_PLUGINS_DIR="`$PKG_CONFIG --variable=pluginsdir gstreamer-1.0`"
AC_SUBST(GST_PLUGINS_DIR)

dnl GStreamer plugin build
plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
AC_SUBST(plugindir)
GST_PLUGIN_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS"
AC_SUBST(GST_PLUGIN_CFLAGS)
GST_PLUGIN_LDFLAGS="-module -avoid-version -export-symbols-regex '[_]*(gst_|bt_|Bt|BT_).*' $COVERAGE_LIBS"
AC_SUBST(GST_PLUGIN_LDFLAGS)
GST_PLUGIN_LIBS="$COVERAGE_LIBS"
AC_SUBST(GST_PLUGIN_LIBS)


dnl check for oil runtime compiler
ORC_CHECK([0.4.6])

dnl check for FluidSynth
PKG_CHECK_MODULES(FLUIDSYNTH, fluidsynth >= 1.1.0,
    [
        AC_DEFINE(HAVE_FLUIDSYNTH, [1], [defined when we have a fluidsynth element])
        have_fluidsynth=yes
    ], [
        have_fluidsynth=no
    ])
AC_SUBST(FLUIDSYNTH_CFLAGS)
AC_SUBST(FLUIDSYNTH_LIBS)
AM_CONDITIONAL(FLUIDSYNTH_SUPPORT, test "x$have_fluidsynth" = "xyes")


dnl set license and copyright notice
AC_DEFINE(PACKAGE_ORIGIN, "http://www.buzztrax.org", [Plugin package origin])
AC_SUBST(PACKAGE_ORIGIN)


dnl Check for gtk-doc.
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.20],[--flavour no-tmpl])
],[
AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])

AC_ARG_ENABLE(man,
              [AS_HELP_STRING([--enable-man],
                              [regenerate man pages from Docbook [default=no]])],enable_man=yes,
              enable_man=no)

AC_PATH_PROG([XSLTPROC], [xsltproc])
AM_CONDITIONAL(ENABLE_MAN, test x$enable_man != xno)

dnl User Documentation
YELP_HELP_INIT

dnl Check for aspell (spellcheck the docs)
AC_CHECK_PROGS(ASPELL,[aspell],[])
AM_CONDITIONAL(WITH_ASPELL,[test "x$ASPELL" != "x"])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS( \
  alloca.h assert.h dirent.h kstat.h malloc.h \
  sys/mman.h sys/time.h sys/times.h \
  X11/Xlocale.h \
)
AC_CHECK_HEADERS([linux/input.h],
  [
    have_linux_input_h=yes
    LINUX_INPUT_DOC_TYPES=
    LINUX_INPUT_DOC_SECTIONS=
  ],
  [
    have_linux_input_h=no
    LINUX_INPUT_DOC_TYPES=%
    LINUX_INPUT_DOC_SECTIONS=#
  ]
)
AC_SUBST(LINUX_INPUT_DOC_TYPES)
AC_SUBST(LINUX_INPUT_DOC_SECTIONS)
AM_CONDITIONAL(USE_LINUX_INPUT, test x$have_linux_input_h == xyes)

AC_CHECK_DECL(sysi86,[
	AC_DEFINE(HAVE_SYSI86,1,[Define this if the 'sysi86' function is declared in sys/sysi86.h])
      ],,[
#include <sys/sysi86.h>
])

dnl check for SSE intrisics
have_sse_intrinsics=no
ARCH_CFLAGS=""
case "x${target_cpu}" in
  xi?86|k?|xx86_64|xamd64)
    AC_CHECK_HEADERS([xmmintrin.h],
      [
        SAVED_CFLAGS="${CFLAGS}"
        AC_MSG_CHECKING([for working sse intrinsics])
        CFLAGS="-mmmx -msse"
        AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
                          #include <xmmintrin.h>
                          int main () {
                              _mm_getcsr ();
                          }]])], [
                          AC_MSG_RESULT(yes)
                          have_sse_intrinsics=yes],[
                          AC_MSG_RESULT(no)])
        CFLAGS="${SAVED_CFLAGS}"
        ARCH_CFLAGS="-mmmx -msse"
      ],[])
    ;;
esac
if test $have_sse_intrinsics = yes ; then
   AC_DEFINE(USE_X86_SSE, 1, [use x86 SSE compiler intrinsics])
   ARCH_CFLAGS="-mmmx -msse"
fi

dnl check for libraries
LT_LIB_M
AC_SUBST(LIBM)
ACX_PTHREAD
AC_CHECK_LIB([rt], [main])

dnl Checks for library functions.
dnl libc functions
AC_CHECK_FUNCS(sched_setscheduler)
AC_CHECK_FUNCS(mlockall)
AC_CHECK_FUNCS(getrusage)
AC_CHECK_FUNCS(setrlimit)
AC_CHECK_FUNCS([vsscanf clock_gettime])

AC_CHECK_FUNC(dlopen,
    [AC_DEFINE(HAVE_LIBDL,1,[We can use libdl functions])],
    [AC_CHECK_LIB(dl, dlopen,
         [
           DL_LIBS=-ldl
           AC_DEFINE(HAVE_LIBDL,1,[We can use libdl functions])
         ],
         [])
    ])

dnl Check for -Bsymbolic-functions linker flag used to avoid
dnl intra-library PLT jumps, if available.
AC_ARG_ENABLE(Bsymbolic,
              [AS_HELP_STRING([--disable-Bsymbolic],
                              [avoid linking with -Bsymbolic])],,
              [SAVED_LDFLAGS="${LDFLAGS}" SAVED_LIBS="${LIBS}"
               AC_MSG_CHECKING([for -Bsymbolic-functions linker flag])
               LDFLAGS=-Wl,-Bsymbolic-functions
               LIBS=
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[ return 0; ]])],[
                           AC_MSG_RESULT(yes)
                           enable_Bsymbolic=yes], [
                           AC_MSG_RESULT(no)
                           enable_Bsymbolic=no])
               LDFLAGS="${SAVED_LDFLAGS}" LIBS="${SAVED_LIBS}"])


dnl Extra vars
BT_INCLUDEDIR='-I${includedir}'
dnl -Wl,--as-needed # can be put into CFLAGS to drop all unused libs
BT_CFLAGS="$ARCH_CFLAGS $DEBUG_CFLAGS $COVERAGE_CFLAGS $BT_DISABLE_DEPRECATED"
BT_LIBDIR='-L${libdir}'
BT_LIBS="$COVERAGE_LIBS"
BT_LDFLAGS="$DEBUG_LDFLAGS"
if test "x${enable_Bsymbolic}" == "xyes"; then
  BT_LDFLAGS="$BT_LDFLAGS -Wl,-Bsymbolic-functions"
fi

BML_CFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS"
# the -Wno-non-virtual-dtor is needed to suppress : virtual methods with no virtual destructor
BML_CXXFLAGS="$DEBUG_CFLAGS $COVERAGE_CFLAGS -Wno-non-virtual-dtor"
BML_LIBS="$COVERAGE_LIBS $DL_LIBS"

dnl check for desktop and mimetype utilities

AC_ARG_ENABLE(update-mime,
	AS_HELP_STRING([--disable-update-mime],[don't run update-mime-database utility (useful for packages) ]))
AM_CONDITIONAL(WITH_UPDATE_MIME, test "$enable_update_mime" != "no")

AC_ARG_ENABLE(update-desktop,
	AS_HELP_STRING([--disable-update-desktop],[don't run update-desktop-database utility (useful for packages) ]))
AM_CONDITIONAL(WITH_UPDATE_DESKTOP, test "$enable_update_desktop" != "no")

AC_ARG_ENABLE(update-icon-cache,
	AS_HELP_STRING([--disable-update-icon-cache],[don't run gtk-update-icon-cache utility (useful for packages) ]))
AM_CONDITIONAL(WITH_UPDATE_ICON_CACHE, test "$enable_update_icon_cache" != "no")

AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)
AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database)
AC_PATH_PROGS(UPDATE_ICON_CACHE, gtk-update-icon-cache, gtk-update-icon-cache-3.0)

dnl Substitutes

AC_SUBST(BT_INCLUDEDIR)
AC_SUBST(BT_CFLAGS)
AC_SUBST(BT_LIBDIR)
AC_SUBST(BT_LIBS)
AC_SUBST(BT_LDFLAGS)
AC_SUBST(BASE_DEPS_LIBS)
AC_SUBST(BASE_DEPS_CFLAGS)
AC_SUBST(GUI_DEPS_LIBS)
AC_SUBST(GUI_DEPS_CFLAGS)
AC_SUBST(BML_CFLAGS)
AC_SUBST(BML_CXXFLAGS)
AC_SUBST(BML_LIBS)
AC_SUBST(BMLHOST_CFLAGS)
AC_SUBST(BMLHOST_CCASFLAGS)
AC_SUBST(ARCH_CFLAGS)
AC_SUBST(ARCH_CCASFLAGS)


AC_CONFIG_FILES(Makefile \
  docs/version.entities \
  docs/help/bt-edit/Makefile \
  docs/help/bt-edit/C/Makefile \
  docs/reference/bt-cmd/Makefile \
  docs/reference/bt-core/Makefile \
  docs/reference/bt-core/buzztrax-core.types \
  docs/reference/bt-edit/Makefile \
  docs/reference/bt-gst/Makefile \
  docs/reference/bt-ic/Makefile \
  docs/reference/bt-ic/buzztrax-ic.types \
  docs/reference/bt-ic/buzztrax-ic-sections.txt \
  po/Makefile.in \
  src/lib/buzztrax.xml \
  src/lib/bml/libbml.pc \
  src/lib/core/libbuzztrax-core.pc \
  src/lib/core/version.h \
  src/lib/gst/libbuzztrax-gst.pc \
  src/lib/ic/libbuzztrax-ic.pc \
  src/lib/ic/version.h \
  tests/bt-cfg.sh \
  tests/lib/core/songio/bsl/bt-cfg.sh \
)
AC_OUTPUT

if test "x${coverage_tool}" != "x"; then
  using_coverage_tool="(using ${coverage_tool})"
fi

echo "
Configuration
	Version                         : ${PACKAGE_VERSION}
	Source code location            : ${srcdir}
	Prefix                          : ${prefix}
	Compiler                        : ${CC}

	Documentation (API)             : ${enable_gtk_doc}
	Documentation (Man)             : ${enable_man}
	Bindings Metadata (GIR)         : ${enable_gir}

	Alsa seq. for interaction ctrl. : ${use_alsa}
	GUdev for interaction ctrl.     : ${use_gudev}
	Linux HID for interaction ctrl. : ${have_linux_input_h}
	FluidSynth support              : ${have_fluidsynth}
	Zipped Song file support (bzt)  : ${use_gsf}
	x86 win32 buzzmachines          : ${enable_dllwrapper}

	Debug                           : ${enable_debug}
	Coverage profiling              : ${enable_coverage} ${using_coverage_tool}
	UI tests on Xvfb                : ${have_xvfb}
	Use of deprecated API           : ${enable_deprecated}
	Check based unit tests          : ${have_check}
	Use valgrind on the tests       : ${have_valgrind}
"

features="${use_alsa}${use_gudev}${use_gsf}${have_fluidsynth}"
if test `echo "$features" | grep "no"` ; then
echo "
Some features are not built. If you like to have them built, please check that
you have the required -devel packages installed and that they can be found in
your PKG_CONFIG_PATH ($PKG_CONFIG_PATH).
"
fi

if test ! `echo "$PKG_CONFIG_PATH:/usr" | tr \: \\n | grep $prefix` ; then
echo "
You are installing buzztrax libraries into a location that is not yet known to
system. In order for other buzztrax modules to find them and the application to
work with all features please register the location in the environment (see
README.md 'installing locally' for details).
"
fi

ORC_OUTPUT
