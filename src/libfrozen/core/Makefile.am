AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST=                                     \
	errors_list.c                           \
        configs/config_parser.tab.h             \
	configs/config_parser.l                 \
	configs/config_parser.y                 

noinst_LTLIBRARIES=libfrozen_core.la
libfrozen_core_la_SOURCES=                                   \
                        libfrozen.c                          \
                        api.c                                \
			errors.c                             \
			data.c                               \
			list.c                               \
                        machine.c                            \
                        configs/config_parser.tab.c          \
                        configs/config_parser.lex.c

libfrozen_core_la_CFLAGS = -I@top_srcdir@/src/libfrozen/data/

headers_confdir=$(headersdir)/configs/
headers_HEADERS=libfrozen.h api.h errors.h data.h list.h machine.h public.h enums.h data_selected.h machine_selected.h
headers_conf_HEADERS=configs/config.h

configs/config_parser.tab.c: configs/config_parser.lex.c
	${BISON} -p config_ -d -b configs/config_parser configs/config_parser.y
configs/config_parser.lex.c:
	${FLEX}  -Pconfig_ --bison-bridge -o configs/config_parser.lex.c configs/config_parser.l

CLEANFILES =	\
		configs/config_parser.tab.c                            \
		configs/config_parser.tab.h                            \
		configs/config_parser.lex.c                            

