include(shop.m4)
include(implode.m4)

{ class = "thread" },
{ class = "emitter", request = {
	request = {
		action = (action_t)"write",
		key    = (raw_t)"testkey",
		value  = (raw_t)"testvalue\n"
	},
	machine = (machine_t){
		SHOP_QUERY(`leveldb'),
		{ class = "end" }
	}
}},

{ class = "emitter", request = {
	request = {
		action = (action_t)"read",
		key    = (raw_t)"testkey",
		value  = (raw_t)"1"
	},
	machine = (machine_t){
		SHOP_QUERY(`leveldb'),
		{ class = "debug" },
		{ class = "data/query", data = (fd_t)"stdout", request = {
			action = (action_t)"write",
			buffer = (env_t)"value"
		}},
		{ class = "end" }
	}
}},
{ class = "kill" },
NULL,

// client
SHOP(`leveldb',
`       IMPLODE(`packed',
	`{
		class  = "modules/zeromq",
		socket = (zeromq_t){
			type = "req",
			connect = "tcp://127.0.0.1:8888"
		},
		buffer = (hashkey_t)"packed"
	}')
')

// server
{ class = "thread", loop = (uint_t)"1" },
{
	class  = "modules/zeromq",
	socket = (zeromq_t){
		type = "rep",
		bind = "tcp://127.0.0.1:8888"
	},
	shop = (machine_t){
		EXPLODE(`buffer',
		`{
			class = "modules/leveldb",
			path  = "test_leveldb/"
		}'),
		{ class = "shop/return" }
	}
},
{ class = "end" }
