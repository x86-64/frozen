AS_IF([test "x$HAVE_FUSE" = "x1"], [ 
	FROZEN_MACHINE(fuse, io/fuse, yes)
])
