noinst_LTLIBRARIES = libfrozen_backend_rewrite.la
libfrozen_backend_rewrite_la_SOURCES = rewrite.c rewrite.h rule_parser.tab.c rule_parser.lex.c rule_parser.tab.h
libfrozen_backend_rewrite_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/

EXTRA_DIST = rule_parser.y rule_parser.l

rule_parser.tab.c: rule_parser.y
	${BISON} -p rewrite_ -d -b rule_parser $<
rule_parser.lex.c: rule_parser.l
	${FLEX}  -P rewrite_ --bison-bridge -o $@ $<

CLEANFILES = rule_parser.tab.c rule_parser.lex.c rule_parser.tab.h
