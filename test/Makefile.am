TESTS          = test
check_PROGRAMS = test

test_SOURCES      = test.c test.h 
test_CFLAGS       = -I$(top_builddir)/src/libfrozen/
test_LDADD        = -lcheck $(top_builddir)/src/libfrozen/libfrozen.la

EXTRA_DIST        = test_valgrind test_gdb test_coverage test_*.c

test.c.m4: tests.m4
test.c: test.c.m4
	${M4} -sP test.c.m4 > $@
tests.m4:
	${LS} test_*.c | ${AWK} '{print "m4_include(" $$0 ")" }' > $@

clean-local:
	rm -rf data_*.dat

CLEANFILES= tests.m4 test.c

