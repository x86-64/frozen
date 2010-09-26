tests_planned=test_backend_file.c test_settings.c test_backend.c 

TESTS          = test
check_PROGRAMS = test
test_LDADD     = -lcheck $(top_builddir)/src/libfrozen.la
test_SOURCES   =  	$(tests_planned)                  \
			test.c test.h                     \
			$(top_builddir)/src/libfrozen.h

