mphf_modules  = mphf_chm_imp.c mphf_chm_imp.h
mphf_modules += mphf_bdz_imp.c mphf_bdz_imp.h

noinst_LTLIBRARIES = libfrozen_machine_mphf.la
libfrozen_machine_mphf_la_SOURCES = mphf.c mphf.h $(mphf_modules)
libfrozen_machine_mphf_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/
