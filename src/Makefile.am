
backends_sources=backends/file.c 

lib_LTLIBRARIES=libfrozen.la
libfrozen_la_CFLAGS= -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__USE_GNU
libfrozen_la_LDFLAGS= -module -avoid-version
libfrozen_la_SOURCES= 	libfrozen.c libfrozen.h \
			list.c      list.h      \
			settings.c  settings.h  \
			$(backends_sources)     \
			backend.c   backend.h



