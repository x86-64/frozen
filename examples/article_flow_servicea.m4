include(daemon.m4)
include(shop.m4)
include(zeromq.m4)

include(article_flow_config.m4)

SHOP(`service_a',
`
	{ class = "assign", before = {                                // add some data to request
		data1 = "service_a"
	}},
	

	{ class = "implode", buffer = (hashkey_t)"buffer" },          // send to socket in request
	{ class = "modules/zeromq",
		socket  = (env_t)"destination",
		input   = (hashkey_t)"buffer",
		convert = (uint_t)"1"
	},
	{ class = "end" }
')

DAEMON(`                                                              // worker code to connect to zeromq router and get requests
	ZEROMQ_HASH_PULL(`SERVICE_A_WORKER_connect', `service_a')
')

{ class = "end" }
