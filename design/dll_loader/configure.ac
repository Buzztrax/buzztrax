dnl Process this file with autoconf to produce a configure script.
dnl Author: Michael Patra   <micky@marie.physik.tu-berlin.de>
dnl                         <patra@itp1.physik.tu-berlin.de>
dnl         Francois Gouget <fgouget@codeweavers.com> for CodeWeavers

AC_REVISION([configure.ac 1.00])
AC_INIT(main.c)

NEEDS_MFC=0

dnl **** Command-line arguments ****

AC_SUBST(OPTIONS)

dnl **** Check for some programs ****

AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LN_S

dnl **** Check for some libraries ****

dnl Check for -lm for BeOS
AC_CHECK_LIB(m,sqrt)
dnl Check for -lw for Solaris
AC_CHECK_LIB(w,iswalnum)
dnl Check for -lnsl for Solaris
AC_CHECK_FUNCS(gethostbyname,, AC_CHECK_LIB(nsl, gethostbyname, X_EXTRA_LIBS="$X_EXTRA_LIBS -lnsl", AC_CHECK_LIB(socket, gethostbyname, X_EXTRA_LIBS="$X_EXTRA_LIBS -lnsl", , -lnsl), -lsocket))
dnl Check for -lsocket for Solaris
AC_CHECK_FUNCS(connect,,AC_CHECK_LIB(socket,connect))

dnl **** Check for gcc strength-reduce bug ****

if test "x${GCC}" = "xyes"
then
  AC_CACHE_CHECK([for gcc strength-reduce bug], ac_cv_c_gcc_strength_bug,
                 AC_TRY_RUN([
int main(void) {
  static int Array[[3]];
  unsigned int B = 3;
  int i;
  for(i=0; i<B; i++) Array[[i]] = i - 3;
  exit( Array[[1]] != -2 );
}],
    ac_cv_c_gcc_strength_bug="no",
    ac_cv_c_gcc_strength_bug="yes",
    ac_cv_c_gcc_strength_bug="yes") )
  if test "$ac_cv_c_gcc_strength_bug" = "yes"
  then
    CFLAGS="$CFLAGS -fno-strength-reduce"
  fi
fi

dnl **** Check for working dll ****

LDSHARED=""
LDXXSHARED=""
LDDLLFLAGS=""
AC_CACHE_CHECK([whether we can build a Linux dll],
               ac_cv_c_dll_linux,
[saved_cflags=$CFLAGS
CFLAGS="$CFLAGS -fPIC -shared -Wl,-soname,conftest.so.1.0,-Bsymbolic"
AC_TRY_LINK(,[return 1],ac_cv_c_dll_linux="yes",ac_cv_c_dll_linux="no")
CFLAGS=$saved_cflags
])
if test "$ac_cv_c_dll_linux" = "yes"
then
  LDSHARED="\$(CC) -shared"
  LDXXSHARED="\$(CXX) -shared"
  LDDLLFLAGS="-Wl,-Bsymbolic"

  AC_CACHE_CHECK([whether the linker accepts -z defs], ac_cv_c_dll_zdefs,
    [saved_cflags=$CFLAGS
    CFLAGS="$CFLAGS -fPIC -shared -Wl,-Bsymbolic,-z,defs"
    AC_TRY_LINK([],[],ac_cv_c_dll_zdefs="yes",ac_cv_c_dll_zdefs="no")
    CFLAGS=$saved_cflags
    ])
  if test "$ac_cv_c_dll_zdefs" = "yes"
  then
    LDDLLFLAGS="$LDDLLFLAGS,-z,defs"
  fi

  AC_CACHE_CHECK([whether the linker accepts -init and -fini], ac_cv_c_dll_init_fini,
    [saved_cflags=$CFLAGS
    CFLAGS="$CFLAGS -fPIC -shared -Wl,-Bsymbolic,-init,__wine_spec_init,-fini,__wine_spec_fini"
    AC_TRY_LINK([],[],ac_cv_c_dll_init_fini="yes",ac_cv_c_dll_init_fini="no")
    CFLAGS=$saved_cflags
  ])
  if test "$ac_cv_c_dll_init_fini" = "yes"
  then
    AC_DEFINE(HAVE_LINKER_INIT_FINI,1,[Define if the linker supports renaming the init and fini functions])
    LDDLLFLAGS="$LDDLLFLAGS,-init,__wine_spec_init,-fini,__wine_spec_fini"
  fi
else
  AC_CACHE_CHECK([whether we can build a UnixWare (Solaris) dll],
                ac_cv_c_dll_unixware,
  [saved_cflags=$CFLAGS
  CFLAGS="$CFLAGS -fPIC -Wl,-G,-h,conftest.so.1.0,-B,symbolic"
  AC_TRY_LINK(,[return 1],ac_cv_c_dll_unixware="yes",ac_cv_c_dll_unixware="no")
  CFLAGS=$saved_cflags
  ])
  if test "$ac_cv_c_dll_unixware" = "yes"
  then
    LDSHARED="\$(CC) -Wl,-G"
    LDXXSHARED="\$(CXX) -Wl,-G"
    LDDLLFLAGS="-Wl,-B,symbolic"
  else
    AC_CACHE_CHECK([whether we can build a NetBSD dll],
                   ac_cv_c_dll_netbsd,
    [saved_cflags=$CFLAGS
    CFLAGS="$CFLAGS -fPIC -Wl,-Bshareable,-Bforcearchive"
    AC_TRY_LINK(,[return 1],ac_cv_c_dll_netbsd="yes",ac_cv_c_dll_netbsd="no")
    CFLAGS=$saved_cflags
    ])
    if test "$ac_cv_c_dll_netbsd" = "yes"
    then
      LDSHARED="\$(CC) -Wl,-Bshareable,-Bforcearchive"
      LDXXSHARED="\$(CXX) -Wl,-Bshareable,-Bforcearchive"
      LDDLLFLAGS="" #FIXME
    else
      AC_CACHE_CHECK([whether we can build a Mach-O (Mac OS X/Darwin) dll],
                   ac_cv_c_dll_macho,
      [saved_cflags=$CFLAGS
      CFLAGS="$CFLAGS -bundle"
      AC_TRY_LINK(,[return 1], ac_cv_c_dll_macho="yes", ac_cv_c_dll_macho="no")
      CFLAGS=$saved_cflags
      ])
      if test "$ac_cv_c_dll_macho" = "yes"
      then
        LDSHARED="\$(CC) -bundle -flat_namespace -undefined suppress"
        LDXXSHARED="\$(CXX) -bundle -flat_namespace -undefined suppress"
        LDDLLFLAGS="-fno-common"
        CFLAGS="$CFLAGS -ffixed-r13 -no-cpp-precomp -Dsocklen_t=u_int32_t"
        CXXFLAGS="$CXXFLAGS -ffixed-r13 -no-cpp-precomp -Dsocklen_t=u_int32_t"
      fi
    fi
  fi
fi
if test "$ac_cv_c_dll_linux" = "no" -a "$ac_cv_c_dll_unixware" = "no" -a "$ac_cv_c_dll_netbsd" = "no" -a "$ac_cv_c_dll_macho" = "no"
then
  AC_MSG_ERROR([Could not find how to build a dynamically linked library])
fi

CFLAGS="$CFLAGS -fPIC"

AC_SUBST(LDSHARED)
AC_SUBST(LDXXSHARED)
AC_SUBST(LDDLLFLAGS)

dnl *** check for the need to define __i386__

AC_CACHE_CHECK([whether we need to define __i386__],ac_cv_cpp_def_i386,
 AC_EGREP_CPP(yes,[#if (defined(i386) || defined(__i386)) && !defined(__i386__)
yes
#endif],
 ac_cv_cpp_def_i386="yes", ac_cv_cpp_def_i386="no"))
if test "$ac_cv_cpp_def_i386" = "yes"
then
    CFLAGS="$CFLAGS -D__i386__"
fi

dnl *** check for the need to define __sparc__

AC_CACHE_CHECK([whether we need to define __sparc__],ac_cv_cpp_def_sparc,
 AC_EGREP_CPP(yes,[#if (defined(sparc) || defined(__sparc)) && !defined(__sparc__)
yes
#endif],
 ac_cv_cpp_def_sparc="yes", ac_cv_cpp_def_sparc="no"))
if test "$ac_cv_cpp_def_sparc" = "yes"
then
    CFLAGS="$CFLAGS -D__sparc__"
    CXXFLAGS="$CXXFLAGS -D__sparc__"
fi

dnl *** check for the need to define __sun__

AC_CACHE_CHECK([whether we need to define __sun__],ac_cv_cpp_def_sun,
 AC_EGREP_CPP(yes,[#if (defined(sun) || defined(__sun)) && !defined(__sun__)
yes
#endif],
 ac_cv_cpp_def_sun="yes", ac_cv_cpp_def_sun="no"))
if test "$ac_cv_cpp_def_sun" = "yes"
then
    CFLAGS="$CFLAGS -D__sun__"
    CXXFLAGS="$CXXFLAGS -D__sun__"
fi

dnl *** check for the need to define __powerpc__

AC_CACHE_CHECK(whether we need to define __powerpc__,ac_cv_cpp_def_powerpc,
 AC_EGREP_CPP(yes,[#if (defined(__ppc__) || defined(__PPC__) || defined(__POWERPC__)) && !defined(__powerpc__)
yes
#endif],
 ac_cv_cpp_def_powerpc="yes", ac_cv_cpp_def_powerpc="no"))
if test "$ac_cv_cpp_def_powerpc" = "yes"
then
    CFLAGS="$CFLAGS -D__powerpc__"
    CXXFLAGS="$CXXFLAGS -D__powerpc__"
fi


dnl **** Test Winelib-related features of the C++ compiler
AC_LANG_CPLUSPLUS()
if test "x${GCC}" = "xyes"
then
  OLDCXXFLAGS="$CXXFLAGS";
  CXXFLAGS="-fpermissive";
  AC_CACHE_CHECK([for g++ -fpermissive option], has_gxx_permissive,
    AC_TRY_COMPILE(,[
        for (int i=0;i<2;i++);
        i=0;
      ],
      [has_gxx_permissive="yes"],
      [has_gxx_permissive="no"])
   )
  CXXFLAGS="-fms-extensions";
  AC_CACHE_CHECK([for g++ -fms-extensions option], has_gxx_msextensions,
    AC_TRY_COMPILE(,[
        ;
      ],
      [has_gxx_msextensions="yes"],
      [has_gxx_msextensions="no"])
   )
  CXXFLAGS="-fno-for-scope";
  AC_CACHE_CHECK([for g++ -fno-for-scope option], has_gxx_no_for_scope,
    AC_TRY_COMPILE(,[
        for (int i=0;i<2;i++);
        i=0;
      ],
      [has_gxx_no_for_scope="yes"],
      [has_gxx_no_for_scope="no"])
   )
  CXXFLAGS="-fshort-wchar";
  AC_CACHE_CHECK([for g++ -fshort-wchar option], has_gxx_short_wchar,
    AC_TRY_COMPILE(,[
        ;
      ],
      [has_gxx_short_wchar="yes"],
      [has_gxx_short_wchar="no"])
  )
  CXXFLAGS="$OLDCXXFLAGS";
  if test "$has_gxx_permissive" = "yes"
  then
    CXXFLAGS="$CXXFLAGS -fpermissive"
  fi
  if test "$has_gxx_msextensions" = "yes"
  then
    CXXFLAGS="$CXXFLAGS -fms-extensions"
  fi
  if test "$has_gxx_no_for_scope" = "yes"
  then
    CXXFLAGS="$CXXFLAGS -fno-for-scope"
  fi
  if test "$has_gxx_short_wchar" = "yes"
  then
    CXXFLAGS="$CXXFLAGS -fshort-wchar"
  fi
fi
AC_LANG_C()

dnl **** Test Winelib-related features of the C compiler
dnl none for now

dnl **** Macros for finding a headers/libraries in a collection of places

dnl AC_PATH_FILE(variable,file,action-if-not-found,default-locations)
AC_DEFUN(AC_PATH_FILE,[
AC_MSG_CHECKING([for $2])
AC_CACHE_VAL(ac_cv_pfile_$1,
[
  ac_found=
  ac_dummy="ifelse([$4], , , [$4])"
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  for ac_dir in $ac_dummy; do
    IFS="$ac_save_ifs"
    if test -z "$ac_dir"
    then
      ac_file="$2"
    else
      ac_file="$ac_dir/$2"
    fi
    if test -f "$ac_file"
    then
      ac_found=1
      ac_cv_pfile_$1="$ac_dir"
      break
    fi
  done
  ifelse([$3],,,[if test -z "$ac_found"
    then
      $3
    fi
  ])
])
$1="$ac_cv_pfile_$1"
if test -n "$ac_found" -o -n "[$]$1"
then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)
])

dnl AC_PATH_HEADER(variable,header,action-if-not-found,default-locations)
dnl Note that the above may set variable to an empty value if the header is
dnl already in the include path
AC_DEFUN(AC_PATH_HEADER,[
AC_MSG_CHECKING([for $2 header])
AC_CACHE_VAL(ac_cv_pheader_$1,
[
  ac_found=
  ac_dummy="ifelse([$4], , :/usr/local/include, [$4])"
  save_CPPFLAGS="$CPPFLAGS"
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  for ac_dir in $ac_dummy; do
    IFS="$ac_save_ifs"
    if test -z "$ac_dir"
    then
      CPPFLAGS="$save_CPPFLAGS"
    else
      CPPFLAGS="-I$ac_dir $save_CPPFLAGS"
    fi
    AC_TRY_COMPILE([#include <$2>],,ac_found=1;ac_cv_pheader_$1="$ac_dir";break)
  done
  CPPFLAGS="$save_CPPFLAGS"
  ifelse([$3],,,[if test -z "$ac_found"
    then
      $3
    fi
  ])
])
$1="$ac_cv_pheader_$1"
if test -n "$ac_found" -o -n "[$]$1"
then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)
])

dnl AC_PATH_LIBRARY(variable,libraries,extra libs,action-if-not-found,default-locations)
AC_DEFUN(AC_PATH_LIBRARY,[
AC_MSG_CHECKING([for $2])
AC_CACHE_VAL(ac_cv_plibrary_$1,
[
  ac_found=
  ac_dummy="ifelse([$5], , :/usr/local/lib, [$5])"
  save_LIBS="$LIBS"
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  for ac_dir in $ac_dummy; do
    IFS="$ac_save_ifs"
    if test -z "$ac_dir"
    then
      LIBS="$2 $3 $save_LIBS"
    else
      LIBS="-L$ac_dir $2 $3 $save_LIBS"
    fi
    AC_TRY_LINK(,,ac_found=1;ac_cv_plibrary_$1="$ac_dir";break)
  done
  LIBS="$save_LIBS"
  ifelse([$4],,,[if test -z "$ac_found"
    then
      $4
    fi
  ])
])
$1="$ac_cv_plibrary_$1"
if test -n "$ac_found" -o -n "[$]$1"
then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)
])

dnl **** Try to find where winelib is located ****

LDPATH=""
WINE_INCLUDE_ROOT=""
WINE_INCLUDE_PATH=""
WINE_LIBRARY_ROOT=""
WINE_LIBRARY_PATH=""
WINE_DLL_ROOT=""
WINE_DLL_PATH=""
WINE_TOOL_PATH=""
WINE=""
WINEBUILD=""
WRC=""

AC_ARG_WITH(wine,
[  --with-wine=DIR           the Wine package (or sources) is in DIR],
[if test "$withval" != "no"; then
  WINE_ROOT="$withval";
  WINE_INCLUDES="";
  WINE_LIBRARIES="";
  WINE_TOOLS="";
else
  WINE_ROOT="";
fi])
if test -n "$WINE_ROOT"
then
  WINE_INCLUDE_ROOT="$WINE_ROOT/include:$WINE_ROOT/include/wine:$WINE_ROOT/include/wine/windows:$WINE_ROOT/include/windows"
  WINE_LIBRARY_ROOT="$WINE_ROOT:$WINE_ROOT/libs:$WINE_ROOT/lib"
  WINE_DLL_ROOT="$WINE_ROOT/dlls:$WINE_ROOT/lib:$WINE_ROOT/lib/wine"
  WINE_TOOL_PATH="$WINE_ROOT:$WINE_ROOT/bin:$WINE_ROOT/tools/wrc:$WINE_ROOT/tools/winebuild"
fi

AC_ARG_WITH(wine-includes,
[  --with-wine-includes=DIR  the Wine includes are in DIR],
[if test "$withval" != "no"; then
  WINE_INCLUDES="$withval";
else
  WINE_INCLUDES="";
fi])
if test -n "$WINE_INCLUDES"
then
  WINE_INCLUDE_ROOT="$WINE_INCLUDES"
fi

AC_ARG_WITH(wine-libraries,
[  --with-wine-libraries=DIR the Wine libraries are in DIR],
[if test "$withval" != "no"; then
  WINE_LIBRARIES="$withval";
else
  WINE_LIBRARIES="";
fi])
if test -n "$WINE_LIBRARIES"
then
  WINE_LIBRARY_ROOT="$WINE_LIBRARIES"
fi

AC_ARG_WITH(wine-dlls,
[  --with-wine-dlls=DIR      the Wine dlls are in DIR],
[if test "$withval" != "no"; then
  WINE_DLLS="$withval";
else
  WINE_DLLS="";
fi])
if test -n "$WINE_DLLS"
then
  WINE_DLL_ROOT="$WINE_DLLS"
fi

AC_ARG_WITH(wine-tools,
[  --with-wine-tools=DIR     the Wine tools are in DIR],
[if test "$withval" != "no"; then
  WINE_TOOLS="$withval";
else
  WINE_TOOLS="";
fi])
if test -n "$WINE_TOOLS"
then
  WINE_TOOL_PATH="$WINE_TOOLS:$WINE_TOOLS/tools/wrc:$WINE_TOOLS/tools/winebuild"
fi

if test -z "$WINE_INCLUDE_ROOT"
then
  WINE_INCLUDE_ROOT=":/usr/include/wine/windows:/usr/include/wine:/usr/local/include/wine/windows:/opt/wine/include/windows:/opt/wine/include/wine";
else
  AC_PATH_FILE(WINE_INCLUDE_ROOT,[windef.h],[
    AC_MSG_ERROR([Could not find the Wine headers (windef.h)])
  ],$WINE_INCLUDE_ROOT)
fi
AC_PATH_HEADER(WINE_INCLUDE_ROOT,[windef.h],[
  AC_MSG_ERROR([Could not include the Wine headers (windef.h)])
],$WINE_INCLUDE_ROOT)
if test -n "$WINE_INCLUDE_ROOT"
then
  WINE_INCLUDE_PATH="-I$WINE_INCLUDE_ROOT"
else
  WINE_INCLUDE_PATH=""
fi

if test -z "$WINE_LIBRARY_ROOT"
then
  WINE_LIBRARY_ROOT=":/usr/lib/wine:/usr/local/lib:/usr/local/lib/wine:/opt/wine/lib"
else
  AC_PATH_FILE(WINE_LIBRARY_ROOT,[libwine.so],
    [
    AC_PATH_FILE(WINE_LIBRARY_ROOT,[libwine.dylib],
      [AC_MSG_ERROR([Could not find the Wine libraries (libwine.dylib or libwine.so)])],
      $WINE_LIBRARY_ROOT)
    ], $WINE_LIBRARY_ROOT)
fi
AC_PATH_LIBRARY(WINE_LIBRARY_ROOT,[-lwine],[],[
  AC_MSG_ERROR([Could not link with the Wine libraries (libwine.so)])
],$WINE_LIBRARY_ROOT)
if test -n "$WINE_LIBRARY_ROOT"
then
  WINE_LIBRARY_PATH="-L$WINE_LIBRARY_ROOT"
  LDPATH="$WINE_LIBRARY_ROOT"
else
  WINE_LIBRARY_PATH=""
fi

save_LIBS="$LIBS"
LIBS="$WINE_LIBRARY_PATH $LIBS"

AC_CHECK_LIB(wine_unicode,wine_cp_wcstombs,[],[
    AC_MSG_ERROR([Could not find the Wine dlls (libwine_unicode.so)])
])

LIBS="$save_LIBS"

if test -z "$WINE_DLL_ROOT"
then
  if test -n "$WINE_LIBRARY_ROOT"
  then
    WINE_DLL_ROOT="$WINE_LIBRARY_ROOT:$WINE_LIBRARY_ROOT/dlls:$WINE_LIBRARY_ROOT/wine"
  else
    WINE_DLL_ROOT="/lib:/lib/wine:/usr/lib:/usr/lib/wine:/usr/local/lib:/usr/local/lib/wine"
  fi
fi

AC_PATH_FILE(WINE_DLL_ROOT,[libntdll.def],[
  AC_MSG_ERROR([Could not find the Wine dlls (libntdll.def)])
],[$WINE_DLL_ROOT])
WINE_DLL_PATH="-L$WINE_DLL_ROOT"
WINE_LIBRARY_PATH="$WINE_LIBRARY_PATH -L$WINE_DLL_ROOT"

if test -z "$WINE_TOOL_PATH"
then
  WINE_TOOL_PATH="$PATH:/usr/local/bin:/opt/wine/bin"
fi
AC_PATH_PROG(WINE,wine,,$WINE_TOOL_PATH)
if test -z "$WINE"
then
  AC_MSG_ERROR([Could not find Wine's wine tool])
fi
AC_PATH_PROG(WINEBUILD,winebuild,,$WINE_TOOL_PATH)
if test -z "$WINEBUILD"
then
  AC_MSG_ERROR([Could not find Wine's winebuild tool])
fi
AC_PATH_PROG(WRC,wrc,,$WINE_TOOL_PATH)
if test -z "$WRC"
then
  AC_MSG_ERROR([Could not find Wine's wrc tool])
fi

case $build_os in
  darwin*|macosx*)
    LDPATH="DYLD_LIBRARY_PATH=\"$LDPATH:\$\$DYLD_LIBRARY_PATH\"";;
  *)
    LDPATH="LD_LIBRARY_PATH=\"$LDPATH:\$\$LD_LIBRARY_PATH\""
esac

AC_SUBST(LDPATH)
AC_SUBST(WINE_INCLUDE_PATH)
AC_SUBST(WINE_LIBRARY_PATH)
AC_SUBST(WINE_DLL_PATH)

dnl **** Try to find where the MFC are located ****
AC_LANG_CPLUSPLUS()

if test "x$NEEDS_MFC" = "x1"
then
  ATL_INCLUDE_ROOT="";
  ATL_INCLUDE_PATH="";
  MFC_INCLUDE_ROOT="";
  MFC_INCLUDE_PATH="";
  MFC_LIBRARY_ROOT="";
  MFC_LIBRARY_PATH="";

  AC_ARG_WITH(mfc,
  [  --with-mfc=DIR            the MFC package (or sources) is in DIR],
  [if test "$withval" != "no"; then
    MFC_ROOT="$withval";
    ATL_INCLUDES="";
    MFC_INCLUDES="";
    MFC_LIBRARIES="";
  else
    MFC_ROOT="";
  fi])
  if test -n "$MFC_ROOT"
  then
    ATL_INCLUDE_ROOT="$MFC_ROOT";
    MFC_INCLUDE_ROOT="$MFC_ROOT";
    MFC_LIBRARY_ROOT="$MFC_ROOT";
  fi

  AC_ARG_WITH(atl-includes,
  [  --with-atl-includes=DIR   the ATL includes are in DIR],
  [if test "$withval" != "no"; then
    ATL_INCLUDES="$withval";
  else
    ATL_INCLUDES="";
  fi])
  if test -n "$ATL_INCLUDES"
  then
    ATL_INCLUDE_ROOT="$ATL_INCLUDES";
  fi

  AC_ARG_WITH(mfc-includes,
  [  --with-mfc-includes=DIR   the MFC includes are in DIR],
  [if test "$withval" != "no"; then
    MFC_INCLUDES="$withval";
  else
    MFC_INCLUDES="";
  fi])
  if test -n "$MFC_INCLUDES"
  then
    MFC_INCLUDE_ROOT="$MFC_INCLUDES";
  fi

  AC_ARG_WITH(mfc-libraries,
  [  --with-mfc-libraries=DIR  the MFC libraries are in DIR],
  [if test "$withval" != "no"; then
    MFC_LIBRARIES="$withval";
  else
    MFC_LIBRARIES="";
  fi])
  if test -n "$MFC_LIBRARIES"
  then
    MFC_LIBRARY_ROOT="$MFC_LIBRARIES";
  fi

  OLDCPPFLAGS="$CPPFLAGS"
  dnl FIXME: We should not have defines in any of the include paths
  CPPFLAGS="$WINE_INCLUDE_PATH -I$WINE_INCLUDE_ROOT/msvcrt -D_DLL -D_MT $CPPFLAGS"
  ATL_INCLUDE_PATH="-I\$(WINE_INCLUDE_ROOT)/msvcrt -D_DLL -D_MT"
  if test -z "$ATL_INCLUDE_ROOT"
  then
    ATL_INCLUDE_ROOT=":$WINE_INCLUDE_ROOT/atl:/usr/include/atl:/usr/local/include/atl:/opt/mfc/include/atl:/opt/atl/include"
  else
    ATL_INCLUDE_ROOT="$ATL_INCLUDE_ROOT:$ATL_INCLUDE_ROOT/atl:$ATL_INCLUDE_ROOT/atl/include"
  fi
  AC_PATH_HEADER(ATL_INCLUDE_ROOT,atldef.h,[
    AC_MSG_ERROR([Could not find the ATL includes])
  ],$ATL_INCLUDE_ROOT)
  if test -n "$ATL_INCLUDE_ROOT"
  then
    ATL_INCLUDE_PATH="$ATL_INCLUDE_PATH -I$ATL_INCLUDE_ROOT"
  fi

  MFC_INCLUDE_PATH="$ATL_INCLUDE_PATH"
  if test -z "$MFC_INCLUDE_ROOT"
  then
    MFC_INCLUDE_ROOT=":$WINE_INCLUDE_ROOT/mfc:/usr/include/mfc:/usr/local/include/mfc:/opt/mfc/include/mfc:/opt/mfc/include"
  else
    MFC_INCLUDE_ROOT="$MFC_INCLUDE_ROOT:$MFC_INCLUDE_ROOT/mfc:$MFC_INCLUDE_ROOT/mfc/include:$MFC_INCLUDE_ROOT/Include"
  fi
  AC_PATH_HEADER(MFC_INCLUDE_ROOT,afx.h,[
    AC_MSG_ERROR([Could not find the MFC includes])
  ],$MFC_INCLUDE_ROOT)
  if test -n "$MFC_INCLUDE_ROOT" -a "$ATL_INCLUDE_ROOT" != "$MFC_INCLUDE_ROOT"
  then
    MFC_INCLUDE_PATH="$MFC_INCLUDE_PATH -I$MFC_INCLUDE_ROOT"
  fi
  CPPFLAGS="$OLDCPPFLAGS"

  if test -z "$MFC_LIBRARY_ROOT"
  then
    MFC_LIBRARY_ROOT=":$WINE_LIBRARY_ROOT:/usr/lib/mfc:/usr/local/lib:/usr/local/lib/mfc:/opt/mfc/lib";
  else
    MFC_LIBRARY_ROOT="$MFC_LIBRARY_ROOT:$MFC_LIBRARY_ROOT/lib:$MFC_LIBRARY_ROOT/mfc/src:$MFC_LIBRARY_ROOT/src";
  fi
  AC_PATH_LIBRARY(MFC_LIBRARY_ROOT,[-lmfc],[$WINE_LIBRARY_PATH -lwine -lwine_unicode],[
    AC_MSG_ERROR([Could not find the MFC library])
  ],$MFC_LIBRARY_ROOT)
  if test -n "$MFC_LIBRARY_ROOT" -a "$MFC_LIBRARY_ROOT" != "$WINE_LIBRARY_ROOT"
  then
    MFC_LIBRARY_PATH="-L$MFC_LIBRARY_ROOT"
  else
    MFC_LIBRARY_PATH=""
  fi

  AC_SUBST(ATL_INCLUDE_PATH)
  AC_SUBST(MFC_INCLUDE_PATH)
  AC_SUBST(MFC_LIBRARY_PATH)
fi

AC_LANG_C()

dnl **** Generate output files ****

MAKE_RULES=Make.rules
AC_SUBST_FILE(MAKE_RULES)

AC_OUTPUT([
Make.rules
Makefile
 ])

echo
echo "Configure finished.  Do 'make' to build the project."
echo

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "autoconf"
dnl End:
