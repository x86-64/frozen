include(shop.m4)
include(daemon.m4)
include(zeromq.m4)

ZEROMQ_SERVICE(`zmq_test1',  `127.0.0.1', `8888')
ZEROMQ_SERVICE(`zmq_test2',  `127.0.0.1', `8889')

{ class = "thread" },

{ class = "assign", before = {
	buffer1 = (raw_t)"Hello, zeromq push world #1!\n",
	buffer2 = (raw_t)"Hello, zeromq push world #2!\n",
	buffer3 = (raw_t)"Hello, zeromq push world #3!\n",
	buffer4 = (raw_t)"Hello, zeromq req world #1!\n",
	buffer5 = (raw_t)"Hello, zeromq req world #2!\n",
	buffer6 = (raw_t)"Hello, zeromq req world #3!\n"
}},

// test zeromq push
ZEROMQ_PUSH(`zmq_test1', `buffer1'),
ZEROMQ_PUSH(`zmq_test1', `buffer2'),
ZEROMQ_PUSH(`zmq_test1', `buffer3'),

// test zeromq req
ZEROMQ_HASH_REQ(`zmq_test2', `buffer = (env_t)"buffer4"'),
ZEROMQ_HASH_REQ(`zmq_test2', `buffer = (env_t)"buffer5"'),
ZEROMQ_HASH_REQ(`zmq_test2', `buffer = (env_t)"buffer6"'),

{ class = "kill" },
NULL,


SHOP(`shop_print',
`
	{ class = "query", data = (env_t)"buffer", request = {
		action      = (action_t)"convert_to",
		destination = (fd_t)"stdout"
	}}
')

DAEMON(`
	ZEROMQ_PULL(`zmq_test1', `shop_print')
')
DAEMON(`
	ZEROMQ_HASH_REP(`zmq_test2', `shop_print')
')


{ class = "end" }
