modules_LTLIBRARIES = module_pcre_t.la
module_pcre_t_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/ -DFROZEN_MODULE
module_pcre_t_la_LDFLAGS = -module -avoid-version -shared -lpcre
module_pcre_t_la_SOURCES = pcre.c
headers_mydirdir=$(headersdir)/modules/pcre_t/
headers_mydir_HEADERS=pcre_t.h
EXTRA_DIST = errors_list.c
