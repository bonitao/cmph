dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(cmph, 0.2)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

dnl Checks for headers
AC_CHECK_HEADERS([getopt.h math.h])

dnl Checks for libraries.
AC_CHECK_LIBM  
LDFLAGS="$LIBM $LDFLAGS"

dnl Checks for library functions.

dnl AC_OUTPUT(Makefile tests/Makefile samples/Makefile)
AC_OUTPUT(Makefile src/Makefile tests/Makefile)
