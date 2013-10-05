modules_LTLIBRARIES = module_ipv4_t.la
module_ipv4_t_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/ -DFROZEN_MODULE
module_ipv4_t_la_LDFLAGS = -module -avoid-version -shared -L@top_srcdir@/src/libfrozen/ -lfrozen
module_ipv4_t_la_SOURCES = main.c
headers_mydirdir=$(headersdir)/modules/ipv4_t/
headers_mydir_HEADERS=ipv4_t.h
EXTRA_DIST = errors_list.c
