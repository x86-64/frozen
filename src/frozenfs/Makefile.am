bin_PROGRAMS     = frozenfs
frozenfs_SOURCES = frozenfs.c
frozenfs_CFLAGS  = @FUSE_CFLAGS@ -I$(top_builddir)/src/libfrozen/core/  -I$(top_srcdir)/src/libfrozen/data/
frozenfs_LDADD   = @FUSE_LIBS@ $(top_builddir)/src/libfrozen/libfrozen.la

