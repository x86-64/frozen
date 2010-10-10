TESTS          = test
check_PROGRAMS = test
test_LDADD     = -lcheck $(top_builddir)/src/libfrozen.la
test_CFLAGS    = -I$(top_builddir)/src/
test_SOURCES   = 	test.c        test.h              \
			$(top_builddir)/src/libfrozen.h
EXTRA_DIST     = test_valgrind test_gdb tests_list.c test_*.c

clean-local:
	rm -rf data_backend_*.dat

