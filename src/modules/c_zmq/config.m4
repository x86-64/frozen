AC_CHECK_LIB(zmq, zmq_init, [HAVE_ZMQ=1], [HAVE_ZMQ=0])
AS_IF([test "x$HAVE_ZMQ" = "x1"], [
	FROZEN_MODULE(c_zmq, yes)
])
