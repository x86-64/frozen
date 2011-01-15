AC_PREREQ(2.61)
AC_INIT([frozen], [0.1.0], [x86mail@gmail.com])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_PROG_LIBTOOL
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])
AC_FUNC_MALLOC

PKG_CHECK_MODULES([CHECK], [check >= 0.8.2])
PKG_CHECK_MODULES([FUSE], [fuse >= 2.6.0])

AC_PATH_PROGS(M4, [gm4 m4], no)
AC_PATH_PROGS(LS, [ls], no)
AC_PATH_PROGS(AWK, [gawk awk], no)
AC_PATH_PROGS(BISON, [bison], no)
AC_PATH_PROGS(FLEX, [flex], no)

AC_ARG_ENABLE(coverage, AS_HELP_STRING([--enable-coverage], [enable coverage testing]))
if test "x${enable_coverage}" = "xyes"; then
	CFLAGS="$CFLAGS --coverage"
fi

AC_ARG_ENABLE(debug,  AS_HELP_STRING([--enable-debug],  [compile with debug symbols]))
if test "x$enable_debug" == "xyes"; then
        CFLAGS="$CFLAGS -g3 -O0 -Wall -Werror -DDEBUG=1"
fi

AC_ARG_ENABLE(profile,  AS_HELP_STRING([--enable-profile],  [enable code profiling]))
if test "x$enable_profile" == "xyes"; then
        CFLAGS="$CFLAGS -pg"
        LDFLAGS="$LDFLAGS -pg"
fi

AC_CONFIG_FILES([Makefile
		doc/Makefile
		src/Makefile
		src/libfrozen/Makefile
		src/frozend/Makefile
		src/frozenfs/Makefile
		src/benchmark/Makefile
		test/Makefile])
AC_OUTPUT
