AC_PREREQ(2.61)
AC_INIT([frozen], [0.1.0], [x86mail@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([CHECK], [check >= 0.8.2])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

PKG_CHECK_MODULES([LIBCONFIGXX], [libconfig++ >= 1.4],,
	AC_MSG_ERROR([libconfig++ 1.4 or newer not found.])
)

AC_CONFIG_FILES([Makefile
		doc/Makefile
		src/Makefile
		test/Makefile])
AC_OUTPUT
