AC_PREREQ(2.61)
AC_INIT([frozen], [0.1.0], [x86mail@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_LIBTOOL
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])
AC_FUNC_MALLOC


PKG_CHECK_MODULES([CHECK], [check >= 0.8.2])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])


PKG_CHECK_MODULES([LIBCONFIGXX], [libconfig++ >= 1.4],,
	AC_MSG_ERROR([libconfig++ 1.4 or newer not found.])
)

CFLAGS="-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__USE_GNU -D_XOPEN_SOURCE=500"

AC_ARG_ENABLE(debug,  AS_HELP_STRING([--enable-debug],  [compile with debug symbols]))
if test "x$enable_debug" == "xyes"; then
        CFLAGS="$CFLAGS -g3 -O0"
fi

AC_ARG_ENABLE(coverage, AS_HELP_STRING([--enable-coverage], [enable coverage testing]))
if test "x${enable_cov}" = "xyes"; then
	CFLAGS="-fprofile-arcs -ftest-coverage $CFLAGS"
fi

AC_ARG_ENABLE(profile,  AS_HELP_STRING([--enable-profile],  [enable code profiling]))
if test "x$enable_profile" == "xyes"; then
        CFLAGS="$CFLAGS -pg"
        LDFLAGS="$LDFLAGS -pg -nopie"
fi

AC_CONFIG_FILES([Makefile
		doc/Makefile
		src/Makefile
		test/Makefile])
AC_OUTPUT
