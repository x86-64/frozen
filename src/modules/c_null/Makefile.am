modules_LTLIBRARIES = module_c_null.la
module_c_null_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/ -DFROZEN_MODULE
module_c_null_la_LDFLAGS = -module -avoid-version -shared -L@top_srcdir@/src/libfrozen/ -lfrozen
module_c_null_la_SOURCES = main.c
EXTRA_DIST = errors_list.c
