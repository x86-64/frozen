include(implode.m4)

define(`ZEROMQ_SERVICE', dnl ZEROMQ_SERVICE(service_name, host, port, hwm)
`	
	define($1`_bind',
	`
		bind    = "tcp://*:$3"
		ifelse($4, `', `',
			`, hwm  = (uint64_t)"$4"'
		)
	')
	define($1`_connect',
	`
		connect = "tcp://$2:$3"
		ifelse($4, `', `',
			`, hwm  = (uint64_t)"$4"'
		)
	')
')

define(`ZEROMQ_SOCKET', dnl ZEROMQ_SOCKET(type, sevice_name)
`(zeromq_t){
	type    = "$1",
	$2
}')

define(`ZEROMQ_LAZY_SOCKET', dnl ZEROMQ_LAZY_SOCKET(type, sevice_name)
`(zeromq_t){
	type    = "$1",
	lazy    = (uint_t)"1",
	$2
}')

define(`ZEROMQ_DEVICE', dnl ZEROMQ_DEVICE(device, frontend_name, backend_name)
`
	{ class  = "modules/zeromq_device",
		device = "$1",
		frontend = (zeromq_t){
			type = "ifelse($1,`queue',`router',ifelse($1,`forwarder',`sub',ifelse($1,`streamer',`pull')))",
			$2
		},
		backend  = (zeromq_t){
			type = "ifelse($1,`queue',`dealer',ifelse($1,`forwarder',`pub',ifelse($1,`streamer',`push')))",
			$3
		}
	}
')

define(`ZEROMQ_HASH_REQ', dnl ZEROMQ_HASH_REQ(service_name, request)
`
        { class = "query",
                request = {
                        action = (action_t)"query",
                        request = {
				$2
                        }
                },
                data = (machine_t){
			IMPLODE(`input', `output',
			`{
				class  = "modules/zeromq",
				socket = ZEROMQ_SOCKET(`req', `$1'),
				input  = (hashkey_t)"input",
				output = (hashkey_t)"output",
				convert = (uint_t)"1"
			}'),
                        { class = "end" }
                }
        }
')

define(`ZEROMQ_HASH_REP', dnl ZEROMQ_HASH_REP(service_name, shop_name)
`
	{ class = "modules/zeromq",
		socket = ZEROMQ_SOCKET(`rep', `$1'),
		shop = (machine_t){
			EXPLODE(`buffer', `buffer',
			`
				SHOP_QUERY($2)
			'),
			SHOP_RETURN()
		}
	}
')

define(`ZEROMQ_HASH_PUSH', dnl ZEROMQ_HASH_PUSH(service_name, buffer_hashkey)
`
	{ class = "query",
		request = {
			action = (action_t)"query",
			request = {
				$2
			}
		},
		data = (machine_t){
			IMPLODE(`input', `output',
			`{
				class   = "modules/zeromq",
				socket  = ZEROMQ_SOCKET(`push', `$1'),
				input   = (hashkey_t)"input",
				output  = (hashkey_t)"output",
				convert = (uint_t)"1"
			}'),
                        { class = "end" }
		}
	}
')

define(`ZEROMQ_HASH_PULL', dnl ZEROMQ_HASH_PULL(service_name, shop_name)
`
	{ class = "modules/zeromq",
		socket = ZEROMQ_SOCKET(`pull', `$1')
	},
	EXPLODE(`buffer', `buffer',
	`
		SHOP_QUERY($2)
	')
')


define(`ZEROMQ_PUSH', dnl ZEROMQ_PUSH(service_name, buffer_hashkey)
`
	{ class = "query",
		request = {
			action = (action_t)"query",
			request = {
				buffer = (env_t)"$2"
			}
		},
		data = (machine_t){
			{
				class   = "modules/zeromq",
				socket  = ZEROMQ_SOCKET(`push', `$1'),
				convert = (uint_t)"1"
			},
			{ class = "end" }
		}
	}
')

define(`ZEROMQ_PULL', dnl ZEROMQ_PULL(service_name, shop_name)
`
	{ class = "modules/zeromq",
		socket = ZEROMQ_SOCKET(`pull', `$1')
	},
	SHOP_QUERY($2)
')

