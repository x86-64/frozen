
backends_sources=backends/locator.c backends/file.c backends/null-proxy.c 

lib_LTLIBRARIES=libfrozen.la
libfrozen_la_CFLAGS= -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__USE_GNU
libfrozen_la_LDFLAGS= -module -avoid-version
libfrozen_la_SOURCES= 	libfrozen.c libfrozen.h \
			list.c      list.h      \
			buffer.c    buffer.h    \
			structs.c   structs.h   \
			settings.c  settings.h  \
			$(backends_sources)     \
			backend.c   backend.h



