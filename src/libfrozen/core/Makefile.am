if HAVE_DL
OPTIONAL = -ldl
endif

AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST=                                     \
	errors_list.c                           \
        configs/config_parser.tab.h             \
	configs/config_parser.l                 \
	configs/config_parser.y                 \
	configs/config_fz_parser.y  

noinst_LTLIBRARIES=libfrozen_core.la
libfrozen_core_la_SOURCES=                                   \
                        libfrozen.c                          \
                        api.c                                \
			errors.c                             \
			data.c                               \
			list.c                               \
                        machine.c                            \
			helpers.c                            \
                        configs/config.c                     \
                        configs/config_fz_parser.tab.c       \
                        configs/config_parser.tab.c          \
                        configs/config_parser.lex.c

libfrozen_core_la_CFLAGS = -I@top_srcdir@/src/libfrozen/data/
libfrozen_core_la_LIBADD = $(OPTIONAL)

headers_confdir=$(headersdir)/configs/
headers_HEADERS=libfrozen.h api.h errors.h data.h list.h machine.h helpers.h public.h enums.h data_selected.h machine_selected.h paths.h
headers_conf_HEADERS=configs/config.h

configs/config_fz_parser.tab.c: configs/config_parser.lex.c
	${BISON} -p config_fz_ -d -b configs/config_fz_parser configs/config_fz_parser.y
configs/config_parser.tab.c: configs/config_parser.lex.c
	${BISON} -p config_ -d -b configs/config_parser configs/config_parser.y
configs/config_parser.lex.c:
	${FLEX}  -Pconfig_ --bison-bridge -o configs/config_parser.lex.c configs/config_parser.l

libfrozen.c: paths.h
paths.h: paths.h.in
	cat paths.h.in | sed "s#%modulesdir%#${modulesdir}#g" | sed "s#%m4sdir%#${m4sdir}#g" > $@

CLEANFILES =	\
		paths.h                                                \
		configs/config_parser.tab.c                            \
		configs/config_parser.tab.h                            \
		configs/config_parser.lex.c                            \
		configs/config_fz_parser.tab.c                         \
		configs/config_fz_parser.tab.h

