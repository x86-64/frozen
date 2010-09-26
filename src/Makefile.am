
backends_sources=backends/file.c 

lib_LTLIBRARIES=libfrozen.la
libfrozen_la_LDFLAGS= -module -avoid-version
libfrozen_la_SOURCES= 	libfrozen.c libfrozen.h \
			list.c      list.h      \
			settings.c  settings.h  \
			$(backends_sources)     \
			backend.c   backend.h



