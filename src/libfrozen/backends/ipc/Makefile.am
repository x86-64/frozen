ipc_modules  = ipc_shmem.c ipc_shmem.h

noinst_LTLIBRARIES = libfrozen_backend_ipc.la
libfrozen_backend_ipc_la_SOURCES = ipc.c ipc.h $(ipc_modules)
libfrozen_backend_ipc_la_CFLAGS = -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/
