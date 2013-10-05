modules_LTLIBRARIES = module_regexp_t.la
module_regexp_t_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/ -DFROZEN_MODULE
module_regexp_t_la_LDFLAGS = -module -avoid-version -shared -L@top_srcdir@/src/libfrozen/ -lfrozen
module_regexp_t_la_SOURCES = regexp.c
headers_mydirdir=$(headersdir)/modules/regexp_t/
headers_mydir_HEADERS=regexp_t.h
EXTRA_DIST = errors_list.c
