dnl Copyright (C) 1996 John W. Eaton <jwe@bevo.che.wisc.edu>
dnl Copyright (C) 1998 Eleftherios Gkioulekas <lf@amath.washington.edu>
dnl  
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software 
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# The following set of macros will allow you to mix Fortran and C or C++
# in a portable manner. This work is based on the autoconf macros written
# by John W. Eaton for GNU Octave, which is also distributed under the 
# terms of the GNU public license, but have been heavily modified by
# Eleftherios Gkioulekas, to make them more generally usable.
# The LF_PATH_F77_LIBS which is the most complicated part of this setup
# is exclusively the work of John Eaton, who has more experience with this
# stuff than I do. Look at newer versions of GNU Octave for improved versions
# of this macro.

# --------------------
# -- Acconfig stuff --
# --------------------

# These macros define the following symbols
dnl ACCONFIG TEMPLATE
dnl #undef F77_APPEND_UNDERSCORE
dnl #undef F77_UPPERCASE_NAMES
dnl END ACCONFIG

# Also, it is important that programs have access to the f77func macro
# which is defined as follows:
dnl ACCONFIG BOTTOM
dnl #ifndef f77func
dnl #  if defined (F77_APPEND_UNDERSCORE)
dnl #    if defined (F77_UPPERCASE_NAMES)
dnl #      define f77func(f, F) F##_
dnl #    else
dnl #      define f77func(f, F) f##_
dnl #    endif
dnl #  else
dnl #    if defined (F77_UPPERCASE_NAMES)
dnl #      define f77func(f, F) F
dnl #    else
dnl #      define f77func(f, F) f
dnl #    endif
dnl #  endif
dnl #endif
dnl END ACCONFIG

# -------------------------------------------------------------------------
# This macro specifies that we want to prefer the proprietary compiler 
# if one is available. The default is to prefer the GNU compilers.
# -------------------------------------------------------------------------

AC_DEFUN(LF_PROG_F77_PREFER_NATIVE_VERSION,[
  lf_f77_prefer_native_version="yes"
])

# -------------------------------------------------------------------------
# This macro specifies that we want to prefer an f2c compatible compiler
# if possible.
# -------------------------------------------------------------------------

AC_DEFUN(LF_PROG_F77_PREFER_F2C_COMPATIBILITY,[
  lf_f77_prefer_native_version="no"
])


# -------------------------------------------------------------------------
# This is the macro that you want to call if you want to use Fortran.
# This macro sets F77 equal to a valid Fortran compiler and FFLAGS
# to a set of flags to pass to that compiler.
# Three options are considered:
# 1) The GNU g77 compiler
# 2) The f2c translator
# 3) The proprietary compiler
# -------------------------------------------------------------------------

AC_DEFUN(LF_PROG_F77,[
  dnl Initialize the following use variables to false
  dnl These variables indicate which compiler we want to use
  lf_f77_use_f2c=false
  lf_f77_use_g77=false
  lf_f77_use_f77=false

  dnl These variable indicates whether we want to build the f2c compiler
  lf_f2c_build_local_copy=false

  dnl Allow the user to force the use of the compiler of his choice
  AC_ARG_WITH(f2c,
    [  --with-f2c              use f2c even if Fortran compiler is available],
    [ if test "$withval" = no
      then
        lf_f77_use_f2c=false
      else
        lf_f77_use_f2c=true
      fi ],
    [ lf_f77_use_f2c=false ])

  AC_ARG_WITH(g77,
    [  --with-g77              use g77 to compile Fortran subroutines],
    [ if test "$withval" = no
      then
        lf_f77_use_g77=false
      else
        lf_f77_use_g77=true
      fi ],
    [ lf_f77_use_g77=false ])

  AC_ARG_WITH(f77,
    [  --with-f77              use f77 to compile Fortran subroutines],
    [ if test "$withval" = no
      then
        lf_f77_use_f77=false
      else
        lf_f77_use_f77=true
      fi ],
    [ lf_f77_use_f77=false ])

  dnl Make sure that only one of the above options for Fortran compilers
  dnl was specified (multiple "no" or --without-FOO options are ok).
  dnl FIXME: still todo

  dnl
  dnl Now assign F77 with the appropriate Fortran compiler 
  dnl 

  dnl Check whether there have been any --with options overriding the
  dnl default behaviour
  if   test "$lf_f77_use_f77" = true
  then
    if test "$with_f77" = yes
    then
      F77=f77
    else
      F77="$with_f77"
    fi
    AC_MSG_RESULT([defining F77 to be $F77])
  elif test "$lf_f77_use_g77" = true
  then 
    if test "$with_g77" = yes
    then
      F77=g77
    else
      F77="$with_g77"
    fi
  AC_MSG_RESULT([defining F77 to be $F77])
  elif test "$lf_f77_use_f2c" = true
  then
    LF_PROG_F2C
  dnl If we are not overriding the default behaviour then go
  dnl ahead with the default behaviour
  else
    dnl Take into account whether we have a preference for a native
    dnl version or the GNU version.
    lf_f77_native_compiler_list="f77 f90 xlf cf77 fc"
    if test "$lf_f77_prefer_native_version" = yes 
    then
      AC_CHECK_PROGS(F77, [ $lf_f77_native_compiler_list g77 ], f2c)
    else
      AC_CHECK_PROGS(F77, [ g77 $lf_f77_native_compiler_list ], f2c)
    fi
    dnl Now update the variables lf_f77_use_[f77|g77|f2c] with the result
    dnl of the behaviour up until now so that we can study and finalize this
    dnl decision later
    if test "$F77" = f2c
    then
      lf_f77_use_f2c=true
    elif test "$F77" = g77
    then
      lf_f77_use_g77=true
    else
      lf_f77_use_f77=true
    fi
    dnl If we couldn't find a native or GNU Fortran compiler, then F77
    dnl will be assigned a temporary value of 'f2c'. In this case, invoke
    dnl the LF_PROG_F2C macro to get a more permanent assignment to F77.
    if test "$F77" = f2c
    then
      LF_PROG_F2C
    fi
  fi
  
  dnl One last paranoid check. It's possible that we're using the GNU Fortran
  dnl compiler, under a name other than 'g77'. For example, perhaps the
  dnl sysadmin symlinked it with 'f77' or something. So one last check to be
  dnl sure
  LF_PROG_F77_IS_G77
  if test "$f77_is_g77" = true
  then
    lf_f77_use_f77=false
    lf_f77_use_f2c=false
    lf_f77_use_g77=true
  fi
 
  dnl At this point we have a correct representation of what kind of
  dnl compiler we selected. Now we must confirm the decision. 
  dnl 1. If we have already decided to use f2c, there is nothing to confirm.
  dnl    This is the most portable way to do it.
  dnl 2. If we are using g77 then just run the canonical additional tests that
  dnl    are needed.
  dnl 3. If we are using f77 then we need to think about it more and perhaps
  dnl    reconsider if f2c compatibility is desired.
  if test "$lf_f77_use_g77" = true
  then
    LF_PROG_NM
    LF_PATH_F77_LIBS
    LF_CHECK_F77_APPEND_UNDERSCORE
    LF_CHECK_F77_UPPERCASE_NAMES
  elif test "$lf_f77_use_f77" = true
  then
    LF_PROG_NM
    LF_PATH_F77_LIBS
    LF_CHECK_F77_APPEND_UNDERSCORE
    LF_CHECK_F77_UPPERCASE_NAMES
    LF_F77_IS_F2C_COMPATIBLE
    dnl Now decide whether to really use this compiler or not
    AC_MSG_CHECKING([whether to use the native compiler])
    if test "$lf_f77_is_f2c_compatible" = no
    then
      if test "$lf_f77_prefer_native_version" = no ||
         test -n "$lf_f77_prefer_native_version"
      then
        AC_MSG_RESULT(no)
        dnl FIXME: Think! Do I need to check for g77 right here?
        LF_PROG_F2C
      fi 
    else
      AC_MSG_RESULT(yes)
    fi
  fi

  dnl Signal to Automake whether we want to build the locally distributed
  dnl version of the f2c compiler
  AM_CONDITIONAL(USE_F2C,test x$lf_f2c_build_local_copy = xtrue)

  dnl By default, compile Fortran with optimization
  FFLAGS="-O2"

  dnl Export F77 and FLAGS to Automake
  AC_SUBST(F77)
  AC_SUBST(FFLAGS)
])

# --------------------------------------------------------------------------
# This macro checks whether the f2c translator is available. If not
# it checks whether the package has included a copy of the f2c oompiler.
# By default if there is a widely installed f2c it is prefered. Otherwise
# we prefer the locally distributed f2c. The --with-local-f2c flag will
# force the locally distributed f2c to be used, if there is a problem with
# the widely distributed one.
# -lF77 -lI77
# --------------------------------------------------------------------------

AC_DEFUN(LF_PROG_F2C,[
  dnl No matter what happens next, we want to link in the math libraries
  dnl Late night fart: Mount, mount, mount...mount the dump tape please.
  FLIBS=""
  AC_CHECK_LIB(m, sin,     [ FLIBS="-lm $FLIBS"])
  AC_CHECK_LIB(ieee, main, [ FLIBS="-lieee $FLIBS"])

  dnl Check whether we have an installed version of 'f2c'
  AC_PATH_PROGS(F2C, f2c F2C, nope)

  dnl If we do have an installed version of 'f2c' check whether we also
  dnl have a copy of the libraries. 

  dnl The first snug is that in order to link the f2c libraries, on many
  dnl systems it is necessary to link in the symbols MAIN_ and MAIN__ on
  dnl which the libraries depend on. So, we need to make a one-night-stand
  dnl such library on the spot.
  rm -f conflib.a conftest.o conftest.c
  cat > conftest.c << EOF
int MAIN_ () { return 0; }
int MAIN__() { return 0; }
EOF
  $CC $CFLAGS -c conftest.c 2>&1 1>&AC_FD_CC
  ar rcu libconflib.a conftest.o 2>&1 1>&AC_FD_CC
  if test -n "$RANLIB"
  then
    $RANLIB libconflib.a 2>&1 1>&AC_FD_CC
  fi
  
  dnl Now, the byzantine test for the f2c libraries.
  dnl We need to check
  dnl 1. The -lf2c -lieee -lm sequence
  dnl 2. The -lF77 -lI77 -lieee -lm sequence
  dnl 3. Flag that the libraries are not available, if we can't find them
  lf_f2c_have_libf2c=false
  lf_f2c_have_libF77=false
  lf_f2c_have_libI77=false
  AC_CHECK_LIB(f2c, f_open, [ lf_f2c_have_libf2c=true ], 
                            [ lf_f2c_have_libf2c=false],
                            [ -L. -lconflib $FLIBS ])
  if test "$lf_f2c_have_libf2c" = false
  then
    AC_CHECK_LIB(F77, d_sin,  [ lf_f2c_have_libF77=true ],
                              [ lf_f2c_have_libF77=false ],
                              [ -L. -lconflib $FLIBS -lm ])
    AC_CHECK_LIB(I77, f_rew,  [ lf_f2c_have_libI77=true ],
                              [ lf_f2c_have_libI77=false ],
                              [ -L. -lconflib -lF77 $FLIBS ])
  fi
  rm -f libconflib.a conftest.o conftest.c

  dnl Now determine whether we have a complete set of libraries
  dnl If we do, then assign FLIBS and flag what type of libraries we have
  lf_f2c_have_libraries=no
  if test "$lf_f2c_have_libf2c" = true
  then
    lf_f2c_have_libraries=yes
    FLIBS="-lf2c $FLIBS"
  else
    if test "$lf_f2c_have_libI77" = true && test "$lf_f2c_have_libF77" = true
    then
      lf_f2c_have_libraries=yes
      FLIBS="-lF77 -lI77 $FLIBS"
    fi
  fi

  dnl Now check whether we need to compile our own f2c
  AC_MSG_CHECKING([whether to use local copy of f2c])
  if test "$F2C" = nope || test "$lf_f2c_have_libraries" = no
  then
    dnl If yes, then check if there is a local copy present
    if test -d fortran && test -d fortran/f2c && test -d fortran/libf2c
    then
      dnl If we do have a local copy present then use it
      lf_f2c_build_local_copy=true
      F2C="`pwd`/fortran/f2c/f2c"
      FLIBS="`pwd`/fortran/libf2c/libf2c.a $FLIBS"
      AC_MSG_RESULT(yes)
    else
      AC_MSG_ERROR([want local version of f2c, but one is not available.])
    fi
  else
    dnl If no, then don't bother compiling the local copy
    AC_MSG_RESULT(no)
  fi

  dnl We want to use the following flags on F2C
  F2CFLAGS="-f -g -A"

  dnl Export the F2C and F2CFLAGS variables to the f2c_comp script
  AC_SUBST(F2C)
  AC_SUBST(F2CFLAGS)

  dnl Export the F77 and FFLAGS symbols to Automake
  F77='$(SHELL) $(top_builddir)/f2c_comp'
  FFLAGS=""
  AC_SUBST(F77)
  AC_SUBST(FFLAGS)
  AC_SUBST(FLIBS)

  dnl The f2c compiler appends underscores but does not use uppercase
  dnl letters. We can not invoke a direct test because the compiler may
  dnl not exist yet. Plus, there's no need to.
  AC_DEFINE(F77_APPEND_UNDERSCORE,1)
])

# ---------------------------------------------------------------------------
# THIS macro tests whether the compiler assigned to F77 is the GNU g77
# compiler. If this is the gnu compiler, then set f77_is_g77 equal to "true".
# Otherwise, it is set to be an empty string.
# ---------------------------------------------------------------------------

AC_DEFUN(LF_PROG_F77_IS_G77,[
  AC_MSG_CHECKING([whether we are using GNU Fortran])
  if AC_TRY_COMMAND([$F77 --version]) | egrep 'GNU Fortran' >/dev/null 2>&1
  then
    f77_is_g77=yes
  else
    f77_is_g77=no 
  fi
  AC_MSG_RESULT([$f77_is_g77])
])

# -------------------------------------------------------------------------
# Check whether the Fortran compiler uses uppercase external names.
# If it does, then we define the macro F77_UPPERCASE_NAMES
# Requires maybe the NM variable to be set to the nm program?
# -------------------------------------------------------------------------

AC_DEFUN(LF_CHECK_F77_UPPERCASE_NAMES,[
  AC_MSG_CHECKING([whether $F77 uses uppercase external names])
  lf_cv_f77_uppercase_names=no
  cat > conftest.f <<EOF
      SUBROUTINE XXYYZZ   
      RETURN
      END
EOF
  if ${F77-f77} -c conftest.f 1>&AC_FD_CC 2>&AC_FD_CC; then
    if test "`$NM conftest.o | grep XXYYZZ`" != ""; then
      lf_cv_f77_uppercase_names=yes
    fi
  fi
  AC_MSG_RESULT([$lf_cv_f77_uppercase_names])
  if test "$lf_cv_f77_uppercase_names" = yes; then
    AC_DEFINE(F77_UPPERCASE_NAMES,1)
  fi
  rm -f conftest.f
])

AC_DEFUN(LF_CHECK_F77_APPEND_UNDERSCORE,[
  AC_MSG_CHECKING([whether $F77 appends underscores to external names])
  lf_cv_f77_append_underscore=no
  cat > conftest.f <<EOF
      SUBROUTINE XXYYZZ   
      RETURN
      END
EOF
  if ${F77-f77} -c conftest.f 1>&AC_FD_CC 2>&AC_FD_CC; then
    if test "$lf_cv_f77_uppercase_names" = yes; then
      if test "`${NM} conftest.o | grep XXYYZZ_`" != ""; then
        lf_cv_f77_append_underscore=yes
      fi
    else
      if test "`${NM} conftest.o | grep xxyyzz_`" != ""; then
        lf_cv_f77_append_underscore=yes
      fi
    fi
  fi
  AC_MSG_RESULT([$lf_cv_f77_append_underscore])
  if test "$lf_cv_f77_append_underscore" = yes; then
    AC_DEFINE(F77_APPEND_UNDERSCORE, 1)
  fi
  rm -f conftest.f
])


# -------------------------------------------------------------------------
# This macro tests whether the compiler assigned to F77 is f2c compatible.
# The answer; "yes" or "no" is stored in lf_f77_is_f2c_compatible
# -------------------------------------------------------------------------

AC_DEFUN(LF_F77_IS_F2C_COMPATIBLE,[
  AC_MSG_CHECKING([whether $F77 is f2c compatible])
  trap 'rm -f ftest* ctest* core; exit 1' 1 3 15
  lf_f77_is_f2c_compatible=no
  cat > ftest.f <<EOF
      INTEGER FUNCTION FORSUB (C, D)
      CHARACTER *(*) C
      INTEGER L
      DOUBLE PRECISION D
      L = LEN (C)
      WRITE (*, '(A,1X,I2)') C(1:L), INT (D)
      FORSUB = 1
      RETURN
      END
EOF

  ${F77-f77} -c ftest.f 1>&AC_FD_CC 2>&AC_FD_CC
  changequote(, )dnl
  cat > ctest.c <<EOF
#include "confdefs.h"
static char s[14];
int main ()
{
  double d = 10.0;
  int len;
  strcpy (s, "FOO-I-HITHERE");
  len = strlen (s);
#ifdef F77_APPEND_UNDERSCORE
  return (! forsub_ (s, &d, len));
#else
  return (! forsub (s, &d, len));
#endif
}
#if defined (sun)
int MAIN_ () { return 0; }
#elif defined (linux) && defined (__ELF__)
int MAIN__ () { return 0; }
#endif
EOF
  changequote([, ])
  if ${CC-cc} -c ctest.c 1>&AC_FD_CC 2>&AC_FD_CC; then
    if ${CC-cc} -o ctest ctest.o ftest.o $FLIBS -lm 1>&AC_FD_CC 2>&AC_FD_CC; then
      ctest_output=`./ctest 2>&1`
      status=$?
      if test $status -eq 0 && test "$ctest_output" = "FOO-I-HITHERE 10"; then
        lf_f77_is_f2c_compatible=yes
      fi
    fi
  fi
  rm -f ftest* ctest* core
  AC_MSG_RESULT($lf_f77_is_f2c_compatible)
])

# --------------------------------------------------------------------------
# See what libraries are used by the Fortran compiler
# Write a minimal program and compiler it with -v. I don't know what to
# do if your compiler doesn't have -v
# You must call LF_HOST_TYPE before calling this macro.
# The result is returned in the variable FLIBS which is made
# available in Makefile.am
# ALSO: requires ac_cv_prog_gcc
# --------------------------------------------------------------------------

AC_DEFUN(LF_PATH_F77_LIBS,[
  AC_MSG_CHECKING(for Fortran libraries)
  dnl
  dnl Write a minimal program and compile it with -v. I don't know
  dnl what to do if your compiler doesn't have -v
  dnl
  changequote(, )dnl
  echo "      END" > conftest.f
  foutput=`${F77-f77} -v -o conftest conftest.f 2>&1`
  dnl
  dnl The easiest thing to do for xlf output is to replace all the commas
  dnl with spaces.  Try to only do that if the output is really from xlf,
  dnl since doing that causes problems on other systems.
  dnl
  xlf_p=`echo $foutput | grep xlfentry`
  if test -n "$xlf_p"; then
    foutput=`echo $foutput | sed 's/,/ /g'`
  fi
  dnl
  ld_run_path=`echo $foutput | \
    sed -n -e 's/^.*LD_RUN_PATH *= *\([^ ]*\).*/\1/p'`
  dnl
  dnl We are only supposed to find this on Solaris systems...
  dnl Uh, the run path should be absolute, shouldn't it?
  dnl
  case "$ld_run_path" in
    /*)
      if test "$ac_cv_prog_gcc" = yes; then
        ld_run_path="-Xlinker -R -Xlinker $ld_run_path"
      else
        ld_run_path="-R $ld_run_path"
      fi
    ;;
    *)
      ld_run_path=
    ;;
  esac
  dnl
  flibs=
  lflags=
  dnl
  dnl If want_arg is set, we know we want the arg to be added to the list,
  dnl so we don't have to examine it.
  dnl
  want_arg=
  dnl
  for arg in $foutput; do
    old_want_arg=$want_arg
    want_arg=
  dnl
  dnl None of the options that take arguments expect the argument to
  dnl start with a -, so pretend we didn't see anything special.
  dnl
    if test -n "$old_want_arg"; then
      case "$arg" in
        -*)
        old_want_arg=
        ;;
      esac
    fi
    case "$old_want_arg" in
      '')
        case $arg in
        /*.a)
          exists=false
          for f in $lflags; do
            if test x$arg = x$f; then
              exists=true
            fi
          done
          if $exists; then
            arg=
          else
            lflags="$lflags $arg"
          fi
        ;;
        -bI:*)
          exists=false
          for f in $lflags; do
            if test x$arg = x$f; then
              exists=true
            fi
          done
          if $exists; then
            arg=
          else
            if test "$ac_cv_prog_gcc" = yes; then
              lflags="$lflags -Xlinker $arg"
            else
              lflags="$lflags $arg"
            fi
          fi
        ;;
        -lang* | -lcrt0.o | -lc | -lgcc)
          arg=
        ;;
        -[lLR])
          want_arg=$arg
          arg=
        ;;
        -[lLR]*)
          exists=false
          for f in $lflags; do
            if test x$arg = x$f; then
              exists=true
            fi
          done
          if $exists; then
            arg=
          else
            case "$arg" in
              -lkernel32)
                case "$canonical_host_type" in
                  *-*-cygwin32)
                  ;;
                  *)
                    lflags="$lflags $arg"
                  ;;
                esac
              ;;
              -lm)
              ;;
              *)
                lflags="$lflags $arg"
              ;;
            esac
          fi
        ;;
        -u)
          want_arg=$arg
          arg=
        ;;
        -Y)
          want_arg=$arg
          arg=
        ;;
        *)
          arg=
        ;;
        esac
      ;;
      -[lLR])
        arg="$old_want_arg $arg"
      ;;
      -u)
        arg="-u $arg"
      ;;
      -Y)
  dnl
  dnl Should probably try to ensure unique directory options here too.
  dnl This probably only applies to Solaris systems, and then will only
  dnl work with gcc...
  dnl
        arg=`echo $arg | sed -e 's%^P,%%'`
        SAVE_IFS=$IFS
        IFS=:
        list=
        for elt in $arg; do
        list="$list -L$elt"
        done
        IFS=$SAVE_IFS
        arg="$list"
      ;;
    esac
  dnl
    if test -n "$arg"; then
      flibs="$flibs $arg"
    fi
  done
  if test -n "$ld_run_path"; then
    flibs_result="$ld_run_path $flibs"
  else
    flibs_result="$flibs"
  fi
  changequote([, ])dnl
  rm -f conftest.f conftest.o conftest
  dnl
  dnl Phew! Done! Now, output the result
  dnl
  FLIBS="$flibs_result"
  AC_MSG_RESULT([$FLIBS])
  AC_SUBST(FLIBS)
])







