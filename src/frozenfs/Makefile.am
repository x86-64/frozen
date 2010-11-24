bin_PROGRAMS     = frozenfs
frozenfs_SOURCES = frozenfs.c
frozenfs_CFLAGS  = @FUSE_CFLAGS@ -I$(top_builddir)/src/libfrozen/ 
frozenfs_LDADD   = @FUSE_LIBS@ $(top_builddir)/src/libfrozen/libfrozen.la

