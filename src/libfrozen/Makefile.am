backends_sources =                           \
	backends/null-proxy.c                \
	backends/file.c                      \
	backends/list.c                      \
	backends/cache.c                     \
	backends/mphf/mphf.c                 \
	backends/mphf/mphf_chm_imp.c         \
	backends/mphf/mphf_bdz_imp.c         \
	backends/mphf/hash_jenkins.c         \
	backends/rewrite/rewrite.c           \
	backends/rewrite/rule_parser.tab.c   \
	backends/rewrite/rule_parser.lex.c   \
	backends/insert-sort/insert-sort.c

#	backends/rewrite/rule_parser.c       \
#	backends/blocks-adressing.c \
#	backends/blocks.c

data_sources= data/void.c     \
	data/raw.c      \
	data/binary.c   \
	data/uint8.c    \
	data/uint16.c   \
	data/uint32.c   \
	data/uint64.c   \
	data/size_t.c   \
	data/off_t.c    \
	data/string.c   \
	data/hash_t.c   \
	data/buffer_t.c \
	data/memory_t.c \
	data/io_t.c     \
	data/chain_t.c

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
			hashkeys.h                              \
			structs.c           structs.h           \
			$(backends_sources)                     \
			backend.c           backend.h           \
			configs/config_parser.tab.c             \
			configs/config_parser.lex.c             \
			public.h

libfrozen.c: data.h data.c
hash.h: hashkeys.h

data.c: ${data_sources}
	${M4} -sP data.c.m4 > $@
data.h: ${data_sources}
	${M4} -sP data.h.m4 > $@
hashkeys.h:
	${M4} -sP hashkeys.h.m4 > $@

configs/config_parser.tab.c:
	${BISON} -p config_ -d -b configs/config_parser configs/config_parser.y
configs/config_parser.lex.c:
	${FLEX}  -P config_ --bison-bridge -o configs/config_parser.lex.c configs/config_parser.l

backends/insert-sort/insert-sort.c:
	${M4} -sP backends/insert-sort/insert-sort.c.m4 > $@

backends/rewrite/rule_parser.tab.c:
	${BISON} -p rewrite_ -d -b backends/rewrite/rule_parser backends/rewrite/rule_parser.y
backends/rewrite/rule_parser.lex.c:
	${FLEX}  -P rewrite_ --bison-bridge -o backends/rewrite/rule_parser.lex.c backends/rewrite/rule_parser.l

backends/rewrite/rewrite.c: backends/rewrite/rule_parser.tab.c backends/rewrite/rule_parser.lex.c
backends/rewrite/rule_parser.lex.c: backends/rewrite/rule_parser.tab.c

data/uint8.c:
	${M4} -sP -DBITS=8  -DNAME=int8  -DDEF=INT8  data/uint.c.m4 > $@
data/uint16.c:
	${M4} -sP -DBITS=16 -DNAME=int16 -DDEF=INT16 data/uint.c.m4 > $@
data/uint32.c:
	${M4} -sP -DBITS=32 -DNAME=int32 -DDEF=INT32 data/uint.c.m4 > $@
data/uint64.c:
	${M4} -sP -DBITS=64 -DNAME=int64 -DDEF=INT64 data/uint.c.m4 > $@
data/size_t.c:
	${M4} -sP -DBITS=64 -DNAME=size_t -DDEF=SIZET data/uint.c.m4 > $@
data/off_t.c:
	${M4} -sP -DBITS=64 -DNAME=off_t -DDEF=OFFT data/uint.c.m4 > $@

CLEANFILES =	backends/locate.c backends/insert-sort/insert-sort.c   \
                backends/rewrite/rule_parser.tab.c                     \
                backends/rewrite/rule_parser.tab.h                     \
                backends/rewrite/rule_parser.lex.c                     \
		configs/config_parser.tab.c                            \
		configs/config_parser.tab.h                            \
		configs/config_parser.lex.c                            \
		data.c data.h                                          \
		hashkeys.h                                             \
		data/uint8.c data/uint16.c data/uint32.c data/uint64.c \
		data/off_t.c data/size_t.c

