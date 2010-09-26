
backends_sources =      \
	backends/file.c \
	backends/oid-locator.c \
	backends/null-proxy.c \
	backends/blocks.c           \
	backends/blocks-adressing.c \
	backends/list.c

data_sources=         \
	data/void.c   \
	data/binary.c \
	data/uint8.c  \
	data/uint16.c \
	data/uint32.c \
	data/uint64.c 

AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST=backends/*.m4 data/*.m4 backends/oids/*.c

lib_LTLIBRARIES=libfrozen.la
libfrozen_la_CFLAGS= -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__USE_GNU -D_XOPEN_SOURCE=500 -Wall -Werror
libfrozen_la_LDFLAGS= -module -avoid-version
libfrozen_la_SOURCES= 	libfrozen.c         libfrozen.h         \
			list.c              list.h              \
			buffer.c            buffer.h            \
			$(data_sources)                         \
			data_protos.c       data_protos.h       \
			data.c              data.h              \
			hash.c              hash.h              \
			structs.c           structs.h           \
			settings.c          settings.h          \
			$(backends_sources)                     \
			backend.c           backend.h           \
			public.h

backends/oid-locator.c: backends/oid-locator.c.m4
	m4 -sP $< > $@

data/uint8.c: data/uint.c.m4
	m4 -sP -DBITS=8 $< > $@
data/uint16.c: data/uint.c.m4
	m4 -sP -DBITS=16 $< > $@
data/uint32.c: data/uint.c.m4
	m4 -sP -DBITS=32 $< > $@
data/uint64.c: data/uint.c.m4
	m4 -sP -DBITS=64 $< > $@

