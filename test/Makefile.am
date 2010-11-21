TESTS          = test
check_PROGRAMS = test

test_SOURCES      = test.c test.h 
test_CFLAGS       = -I$(top_builddir)/src/libfrozen/
test_LDADD        = -lcheck $(top_builddir)/src/libfrozen/libfrozen.la

EXTRA_DIST        = test_valgrind test_gdb tests_list.c test_*.c

clean-local:
	rm -rf data_*.dat

