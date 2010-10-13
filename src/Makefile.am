
backends_sources =                  \
	backends/null-proxy.c       \
	backends/file.c            
	
#	backends/oid-locator.c      \
#	backends/blocks.c           \
#	backends/blocks-adressing.c \
#	backends/list.c             \
#	backends/insert-sort.c

data_sources=         \
	data/void.c   \
	data/binary.c \
	data/uint8.c  \
	data/uint16.c \
	data/uint32.c \
	data/uint64.c \
	data/string.c 

AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST=backends/*.m4 data/*.c data/*.m4 backends/insert-sort/*.c

lib_LTLIBRARIES=libfrozen.la
libfrozen_la_LDFLAGS= -module -avoid-version
libfrozen_la_CPPFLAGS= -fvisibility=hidden
libfrozen_la_SOURCES=   data.h \
			libfrozen.c         libfrozen.h         \
			list.c              list.h              \
			buffer.c            buffer.h            \
			data_protos.m4                          \
			data.c                                  \
			hash.c              hash.h              \
			structs.c           structs.h           \
			settings.c          settings.h          \
			$(backends_sources)                     \
			backend.c           backend.h           \
			public.h

libfrozen.c: data.h data.c
data.c: ${data_sources}
	${M4} -sP data.c.m4 > $@
data.h: ${data_sources}
	${M4} -sP data.h.m4 > $@

backends/oid-locator.c: 
	${M4} -sP backends/oid-locator.c.m4 > $@
backends/insert-sort.c:
	${M4} -sP backends/insert-sort.c.m4 > $@

data/uint8.c:
	${M4} -sP -DBITS=8  data/uint.c.m4 > $@
data/uint16.c:
	${M4} -sP -DBITS=16 data/uint.c.m4 > $@
data/uint32.c:
	${M4} -sP -DBITS=32 data/uint.c.m4 > $@
data/uint64.c:
	${M4} -sP -DBITS=64 data/uint.c.m4 > $@

CLEANFILES= backends/oid-locator.c backends/insert-sort.c          \
	    data.c data.h                                          \
            data/uint8.c data/uint16.c data/uint32.c data/uint64.c

