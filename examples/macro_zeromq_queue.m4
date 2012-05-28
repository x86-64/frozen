include(shop.m4)
include(daemon.m4)
include(zeromq.m4)

ZEROMQ_QUEUE(`myqueue', `zmq_local_queue', `5')

SHOP(`shop_print',
`
	{ class = "query", data = (env_t)"buffer", request = {
		action      = (action_t)"convert_to",
		destination = (fd_t)"stdout"
	}}
')

DAEMON(`
	ZEROMQ_PULL(`myqueue_bind', `shop_print')
')


{ class = "assign", before = {
	buffer1 = (raw_t)"Hello, zeromq push world!\n"
}},

// test zeromq push
ZEROMQ_PUSH(`myqueue_connect', `buffer1'),

{ class = "delay", sleep = (uint_t)"1" },
{ class = "end" }

