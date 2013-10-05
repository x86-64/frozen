modules_LTLIBRARIES = module_mynull_t.la
module_mynull_t_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/ -DFROZEN_MODULE
module_mynull_t_la_LDFLAGS = -module -avoid-version -shared -L@top_srcdir@/src/libfrozen/ -lfrozen
module_mynull_t_la_SOURCES = main.c
EXTRA_DIST = errors_list.c
