tests_planned=test_20_backend_locator.c test_19_backend.c test_11_backend_file.c test_00_settings.c

TESTS          = test
check_PROGRAMS = test
test_LDADD     = -lcheck $(top_builddir)/src/libfrozen.la
test_CFLAGS    = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__USE_GNU
test_SOURCES   =  	$(tests_planned)                  \
			test.c test.h                     \
			$(top_builddir)/src/libfrozen.h

