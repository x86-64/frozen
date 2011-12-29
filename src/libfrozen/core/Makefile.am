AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST=                                     \
	errors_list.c                           \
	hashkeys_int.h                          \
	configs/config_parser.l                 \
	configs/config_parser.y                 

noinst_LTLIBRARIES=libfrozen_core.la
libfrozen_core_la_SOURCES=  \
                        libfrozen.h     libfrozen.c                          \
                        api.h           api.c                                \
			errors.h        errors.c                             \
			data.h          data.c                               \
			list.h          list.c                               \
                        buffer.h        buffer.c                             \
                        backend.h       backend.c                            \
                        configs/config.h                        \
                        configs/config_parser.tab.h             \
                        configs/config_parser.tab.c             \
                        configs/config_parser.lex.c             \
                        public.h
libfrozen_core_la_CFLAGS = -I@top_srcdir@/src/libfrozen/data/

configs/config_parser.tab.c: configs/config_parser.lex.c
	${BISON} -p config_ -d -b configs/config_parser configs/config_parser.y
configs/config_parser.lex.c:
	${FLEX}  -Pconfig_ --bison-bridge -o configs/config_parser.lex.c configs/config_parser.l

CLEANFILES =	\
		configs/config_parser.tab.c                            \
		configs/config_parser.tab.h                            \
		configs/config_parser.lex.c                            

