noinst_LTLIBRARIES = libfrozen_backend_insert_sort.la
libfrozen_backend_insert_sort_la_SOURCES = insert_sort.c
libfrozen_backend_insert_sort_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/

insert_sort.c: insert_sort.m4
	${M4} -sP $< > $@

CLEANFILES = insert_sort.c
