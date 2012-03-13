include(shop.m4)
include(daemon.m4)
include(zeromq.m4)

ZEROMQ_SERVICE(`zmq_test1',  `127.0.0.1', `8888', `1000')
ZEROMQ_SERVICE(`zmq_test2',  `127.0.0.1', `8889')
ZEROMQ_SERVICE(`zmq_test3',  `127.0.0.1', `8892')
ZEROMQ_SERVICE(`zmq_dev_in', `127.0.0.1', `8890')
ZEROMQ_SERVICE(`zmq_dev_out',`127.0.0.1', `8891')

{ class = "thread" },

{ class = "assign", before = {
	buffer1 = (raw_t)"Hello, zeromq push world #1!\n",
	buffer2 = (raw_t)"Hello, zeromq push world #2!\n",
	buffer3 = (raw_t)"Hello, zeromq push world #3!\n",
	
	buffer4 = (raw_t)"Hello, zeromq req world #1!\n",
	buffer5 = (raw_t)"Hello, zeromq req world #2!\n",
	buffer6 = (raw_t)"Hello, zeromq req world #3!\n",
	
	buffer7 = (raw_t)"Hello, zeromq device world #1!\n",
	buffer8 = (raw_t)"Hello, zeromq device world #2!\n",
	buffer9 = (raw_t)"Hello, zeromq device world #3!\n",
	
	buffer10 = (raw_t)"Hello, zeromq push hash world #1!\n",
	buffer11 = (raw_t)"Hello, zeromq push hash world #2!\n",
	buffer12 = (raw_t)"Hello, zeromq push hash world #3!\n"
}},

// test zeromq push
ZEROMQ_PUSH(`zmq_test1_connect', `buffer1'),
ZEROMQ_PUSH(`zmq_test1_connect', `buffer2'),
ZEROMQ_PUSH(`zmq_test1_connect', `buffer3'),

ZEROMQ_PUSH(`zmq_dev_in_connect', `buffer7'),
ZEROMQ_PUSH(`zmq_dev_in_connect', `buffer8'),
ZEROMQ_PUSH(`zmq_dev_in_connect', `buffer9'),

ZEROMQ_HASH_PUSH(`zmq_test3_connect', `buffer = (env_t)"buffer10"'),
ZEROMQ_HASH_PUSH(`zmq_test3_connect', `buffer = (env_t)"buffer11"'),
ZEROMQ_HASH_PUSH(`zmq_test3_connect', `buffer = (env_t)"buffer12"'),

// test zeromq req
ZEROMQ_HASH_REQ(`zmq_test2_connect', `buffer = (env_t)"buffer4"'),
ZEROMQ_HASH_REQ(`zmq_test2_connect', `buffer = (env_t)"buffer5"'),
ZEROMQ_HASH_REQ(`zmq_test2_connect', `buffer = (env_t)"buffer6"'),

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
	ZEROMQ_PULL(`zmq_test1_bind', `shop_print')
')
DAEMON(`
	ZEROMQ_HASH_REP(`zmq_test2_bind', `shop_print')
')
DAEMON(`
	ZEROMQ_HASH_PULL(`zmq_test3_bind', `shop_print')
')

// device
DAEMON(`
	ZEROMQ_DEVICE(`streamer', `zmq_dev_in_bind', `zmq_dev_out_bind')
')
DAEMON(`
	ZEROMQ_PULL(`zmq_dev_out_connect', `shop_print')
')

{ class = "end" }
