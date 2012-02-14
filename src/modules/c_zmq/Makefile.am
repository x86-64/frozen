modules_LTLIBRARIES = module_c_zmq.la
module_c_zmq_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/ -DFROZEN_MODULE
module_c_zmq_la_LDFLAGS = -module -avoid-version -shared -lzmq
module_c_zmq_la_SOURCES = main.c
headers_mydirdir=$(headersdir)/modules/zeromq_t/
headers_mydir_HEADERS=zeromq_t.h
EXTRA_DIST = errors_list.c
