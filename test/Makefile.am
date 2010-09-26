tests_planned=test_backend_file.c test_backend.c 

TESTS               = check_main
check_PROGRAMS      = check_main
check_main_LDADD    = -lcheck $(top_builddir)/src/libfrozen.la
check_main_SOURCES  =  	$(tests_planned)                  \
			check_main.c check_main.h         \
			$(top_builddir)/src/libfrozen.h

