AS_IF([test "x$HAVE_FUSE" = "x1"], [ 
	FROZEN_BACKEND(fuse, io/fuse, yes)
])
