dnl
dnl This is the configure.in for faad2 related to unix creation

dnl - libfaad.so
dnl - faad

AC_PREREQ(2.50)
dnl DO NOT EDIT VERSION; actual version is substituted by bootstrap
AC_INIT(faad2, 2.10.1)
AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE([subdir-objects])

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_CHECK_PROGS(RPMBUILD, rpmbuild, rpm)

AC_CONFIG_HEADERS([config.h])

AC_ARG_WITH(drm,[  --with-drm              compile libfaad with DRM support],
	     WITHDRM=$withval, WITHDRM=no)

AC_ARG_WITH(drc,[  --with-drc              compile libfaad with DRC support],
	     WITHDRC=$withval, WITHDRC=yes)

dnl Checks for header files required for mp4.h
AC_HEADER_STDC
AC_CHECK_HEADERS(stdint.h inttypes.h)
AC_CHECK_FUNCS(strchr memcpy)
AC_HEADER_TIME

dnl DRMS 
AC_CHECK_HEADERS(sys/stat.h sys/types.h)

AC_C_INLINE
AC_C_BIGENDIAN

AC_TYPE_OFF_T

AC_DEFUN([MY_CHECK_TYPEDEF_FROM_INCLUDE],
[
   AC_MSG_CHECKING([for $1])
   AC_TRY_COMPILE([$2],
                  [$1;],
                  libfaad_ok=yes, libfaad_ok=no)
   if test $libfaad_ok = yes; then
      AC_DEFINE($3, 1, [Define if needed])
      AC_MSG_RESULT([yes])
   else
      AC_MSG_RESULT([no])
fi
])

dnl @synopsis AC_C99_FUNC_LRINTF
dnl
dnl Check whether C99's lrintf function is available.
dnl @version 1.3        Feb 12 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any
dnl purpose is hereby granted without fee, provided that the above copyright
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LRINTF],
[AC_CACHE_CHECK(for lrintf,
  ac_cv_c99_lrintf,
[
lrintf_save_CFLAGS=$CFLAGS
lrintf_save_LIBS=$LIBS
CFLAGS="-O"
LIBS="-lm"
AC_TRY_LINK([
#define         _ISOC9X_SOURCE  1
#define         _ISOC99_SOURCE  1
#define         __USE_ISOC99    1
#define         __USE_ISOC9X    1

#include <math.h>
], if (!lrintf(3.14159)) lrintf(2.7183);, ac_cv_c99_lrintf=yes, ac_cv_c99_lrintf=no)

CFLAGS=$lrintf_save_CFLAGS
LIBS=$lrintf_save_LIBS

])

if test "$ac_cv_c99_lrintf" = yes; then
  AC_DEFINE(HAVE_LRINTF, 1,
            [Define if you have C99's lrintf function.])
fi
])# AC_C99_FUNC_LRINTF
AC_C99_FUNC_LRINTF

MY_CHECK_TYPEDEF_FROM_INCLUDE([float32_t temp],
        [#include <sys/types.h>,
        #include <sys/float.h>], [HAVE_FLOAT32_T])

AC_CHECK_FUNCS(strsep)

if test x$WITHDRM = xyes; then
  AC_DEFINE(DRM_SUPPORT, 1, [Define if you want to use libfaad together with Digital Radio Mondiale (DRM)])
fi

if test x$WITHDRC = xyes; then
  AC_DEFINE(APPLY_DRC, 1, [Define if you want to process Dynamic Range Compression (DRC)])
fi

AC_CONFIG_FILES(libfaad/Makefile)
AC_CONFIG_FILES(libfaad/faad2.pc)
AC_CONFIG_FILES(include/faad.h)
AC_CONFIG_FILES(faad2.spec)
AC_CONFIG_FILES(frontend/Makefile)
AC_CONFIG_FILES(Makefile)

AC_OUTPUT
