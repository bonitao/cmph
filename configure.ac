dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE(cmph, 1.0)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
LT_INIT
AC_SYS_EXTRA_LARGEFILE
if test "x$ac_cv_sys_largefile_CFLAGS" = "xno" ; then
	ac_cv_sys_largefile_CFLAGS=""
fi
if test "x$ac_cv_sys_largefile_LDFLAGS" = "xno" ; then
	ac_cv_sys_largefile_LDFLAGS=""
fi
if test "x$ac_cv_sys_largefile_LIBS" = "xno" ; then
	ac_cv_sys_largefile_LIBS=""
fi
CFLAGS="$CFLAGS $ac_cv_sys_largefile_CFLAGS"
LDFLAGS="$LDFLAGS $ac_cv_sys_largefile_LDFLAGS"
LIBS="$LIBS $ac_cv_sys_largefile_LIBS"

dnl Checks for headers
AC_CHECK_HEADERS([getopt.h math.h])

dnl Checks for libraries.
LT_LIB_M  
LDFLAGS="$LIBM $LDFLAGS"
CFLAGS="-Wall -Werror"

AC_PROG_CXX
AC_ENABLE_CXXMPH
if test x$cxxmph = xtrue; then
	AC_SUBST([CXXMPH], "cxxmph")
fi

AC_CHECK_SPOON
dnl AC_CONFIG_FILES([Makefile tests/Makefile samples/Makefile])
AC_OUTPUT
AC_CONFIG_FILES([Makefile src/Makefile cxxmph/Makefile tests/Makefile examples/Makefile man/Makefile cmph.pc])
AC_OUTPUT
