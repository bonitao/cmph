dnl Process this file with autoconf to produce a configure script.
AC_INIT([cmph],[2.0.2])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE
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
CFLAGS="$ac_cv_sys_largefile_CFLAGS $CFLAGS"
LDFLAGS="$ac_cv_sys_largefile_LDFLAGS $LDFLAGS"
LIBS="$LIBS $ac_cv_sys_largefile_LIBS"

dnl Probe for hardware CRC32C intrinsics. The probe compiles a TU that
dnl references _mm_crc32_u8; on x86 this comes from <smmintrin.h> when
dnl SSE4.2 is available, on aarch64 it would come from a bundled
dnl sse2neon.h shim (not yet shipped). When the probe fails, the
dnl crc32_hash translation unit falls back to its software LUT.
AC_CACHE_CHECK([for crc32c intrinsics],
  ac_cv_crc32_hw, [
  ac_cv_crc32_hw=no
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[
#if defined(__aarch64__)
# include "sse2neon.h"
#elif defined __SSE4_2__ &&                                           \
    (defined __x86_64__ || defined __i686_64__ || defined _M_AMD64 || \
	defined _M_IX86)
# include <smmintrin.h>
#endif]], [[
(void)_mm_crc32_u8(0L, 0L);
]])],
    [ac_cv_crc32_hw=yes])
  ])
if test x$ac_cv_crc32_hw = xyes;
then
  AC_DEFINE(HAVE_CRC32_HW, 1,
    [Define if crc32c-ni is supported.])
fi
AM_CONDITIONAL([CRC32_HW], [test x$ac_cv_crc32_hw = xyes])

dnl Checks for headers
AC_CHECK_HEADERS([getopt.h math.h])

dnl Checks for libraries.
LT_LIB_M
LDFLAGS="$LIBS $LIBM $LDFLAGS"
CFLAGS="-Wall $CFLAGS"

AC_PROG_CXX
CXXFLAGS="-Wall -Wno-unused-function -DNDEBUG -O3 -fomit-frame-pointer $CXXFLAGS"
AC_ENABLE_CXXMPH
if test x$cxxmph = xtrue; then
  AC_COMPILE_STDCXX_0X
  if test x$ac_cv_cxx_compile_cxx0x_native = "xno"; then
    if test x$ac_cv_cxx_compile_cxx11_cxx = "xyes"; then
      CXXFLAGS="$CXXFLAGS -std=c++11"
    elif test x$ac_cv_cxx_compile_cxx0x_cxx = "xyes"; then
      CXXFLAGS="$CXXFLAGS -std=c++0x"
    elif test x$ac_cv_cxx_compile_cxx0x_gxx = "xyes"; then
      CXXFLAGS="$CXXFLAGS -std=gnu++0x"
    else
      AC_MSG_ERROR("cxxmph demands a working c++0x compiler.")
    fi
  fi
  AC_SUBST([CXXMPH], "cxxmph")
fi
AM_CONDITIONAL([USE_CXXMPH], [test "$cxxmph" = true])

AC_ENABLE_BENCHMARKS
if test x$benchmarks = xtrue; then
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADERS([hopscotch_map.h])
  AC_LANG_POP([C++])
fi
AM_CONDITIONAL([USE_BENCHMARKS], [test "$benchmarks" = true])

# Unit tests based on the check library. Disabled by default.
# We do not use pkg-config because it is inconvenient for all developers to
# have check library installed.
AC_ARG_ENABLE(check, AS_HELP_STRING(
     [--enable-check],
     [Build unit tests depending on check library (default: disabled)]))
AS_IF([test "x$enable_check" = "xyes"],
  [ AC_CHECK_LIB([check], [tcase_create])
    AS_IF([test "$ac_cv_lib_check_tcase_create" = yes], [CHECK_LIBS="-lcheck"],
          [AC_MSG_ERROR("Failed to find check library (http://check.sf.net).")])
    AC_CHECK_HEADER(check.h,[],
          [AC_MSG_ERROR("Failed to find check library header (http://check.sf.net).")])
  ])
AM_CONDITIONAL([USE_LIBCHECK], [test "$ac_cv_lib_check_tcase_create" = yes])
AC_SUBST(CHECK_LIBS)
AC_SUBST(CHECK_CFLAGS)

AC_CHECK_SPOON
AC_CONFIG_FILES([Makefile src/Makefile cxxmph/Makefile tests/Makefile examples/Makefile man/Makefile cmph.pc cxxmph.pc])
AC_OUTPUT
