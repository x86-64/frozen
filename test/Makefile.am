TESTS          = test
check_PROGRAMS = test
test_LDADD     = -lcheck $(top_builddir)/src/libfrozen.la
test_CFLAGS    = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__USE_GNU -O0 -I$(top_builddir)/src/
test_SOURCES   = 	test.c        test.h              \
			$(top_builddir)/src/libfrozen.h
EXTRA_DIST     = test_valgrind tests_list.c test_*.c
