modules_LTLIBRARIES = module_time_t.la
module_time_t_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/ -DFROZEN_MODULE
module_time_t_la_LDFLAGS = -module -avoid-version -shared -lfrozen
module_time_t_la_SOURCES = main.c
headers_mydirdir=$(headersdir)/modules/time_t/
headers_mydir_HEADERS=time_t.h
EXTRA_DIST = errors_list.c
