mphf_modules  = hash_jenkins.c hash_jenkins.h
mphf_modules += mphf_chm_imp.c mphf_chm_imp.h
mphf_modules += mphf_bdz_imp.c mphf_bdz_imp.h

noinst_LTLIBRARIES = libfrozen_backend_mphf.la
libfrozen_backend_mphf_la_SOURCES = mphf.c mphf.h $(mphf_modules)
libfrozen_backend_mphf_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/
