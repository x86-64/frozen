include(implode.m4)

define(`ZEROMQ_SERVICE', dnl ZEROMQ_SERVICE(service_name, host, port, hwm)
`	define($1`_HOST', $2)dnl
	define($1`_PORT', $3)dnl
	define($1`_LISTEN', `tcp://*:'$3)dnl
	ifelse($4, `', `', `define(`$1_HWM', $4)')dnl
	define($1, `tcp://'$2`:'$3)dnl
')

define(`ZEROMQ_HASH_REP', dnl ZEROMQ_HASH_REP(service_name, shop_name)
`
	{ class = "modules/zeromq",
		socket = (zeromq_t){
			type = "rep",
			bind = "$1_LISTEN"
			ifdef(`$1_HWM', `, hwm  = (uint64_t)"$1_HWM"')
		},
		shop = (machine_t){
			EXPLODE(`buffer', `buffer',
			`
				SHOP_QUERY($2)
			'),
			SHOP_RETURN()
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
				socket = (zeromq_t){
					type    = "req",
					connect = "$1"
					ifdef(`$1_HWM', `, hwm  = (uint64_t)"$1_HWM"')
				},
				input  = (hashkey_t)"input",
				output = (hashkey_t)"output",
				convert = (uint_t)"1"
			}'),
                        { class = "end" }
                }
        }
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
				class  = "modules/zeromq",
				socket = (zeromq_t){
					type    = "push",
					connect = "$1"
					ifdef(`$1_HWM', `, hwm  = (uint64_t)"$1_HWM"')
				},
				convert = (uint_t)"1"
			},
			{ class = "end" }
		}
	}
')

define(`ZEROMQ_PULL', dnl ZEROMQ_PULL(service_name, shop_name)
`
	{ class = "modules/zeromq",
		socket = (zeromq_t){
			type = "pull",
			bind = "$1_LISTEN"
			ifdef(`$1_HWM', `, hwm  = (uint64_t)"$1_HWM"')
		}
	},
	SHOP_QUERY($2)
')

