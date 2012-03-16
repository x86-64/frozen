include(daemon.m4)
include(shop.m4)
include(zeromq.m4)

include(article_flow_config.m4)

SHOP(`service_b',
`
	{ class = "assign", before = {                                 // add some data here too
		data2 = "service_b"
	}},
	
	{ class = "query",                                             // print all results
		data = (container_t){
			"client data: ",        (env_t)"data",
			"\nservice a data: ",   (env_t)"data1",
			"\nservice b data: ",   (env_t)"data2",
			"\n"
		},
		request = {
			action      = (action_t)"convert_to",
			destination = (fd_t)"stdout"
		}
	}
')

DAEMON(`                                                               // worker code to connect to zeromq router and get requests
	ZEROMQ_HASH_PULL(`SERVICE_B_WORKER_connect', `service_b')
')

{ class = "end" }
