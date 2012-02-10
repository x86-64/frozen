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
		class   = "data/query",
		data    = (named_t){
			name = "leveldb_req",
			data = (zeromq_t){
				type = "req",
				connect = "tcp://127.0.0.1:8888"
			}
		},
		request = {
			action = (action_t)"write",
			buffer = (env_t)"packed"
		}
	},
	{
		class   = "data/query",
		data    = (named_t)"leveldb_req",
		request = {
			action      = (action_t)"transfer",
			destination = (env_t)"packed"
		}
	}')
')

// server
{ class = "thread", loop = (uint_t)"1" },
{ class = "query",
	data = (named_t){
		name = "leveldb_rep",
		data = (zeromq_t){
			type = "rep",
			bind = "tcp://127.0.0.1:8888"
		}
	},
	request = {
		action = (action_t)"transfer",
		destination = (machine_t){
			EXPLODE(`buffer',
			`{
				class = "modules/leveldb",
				path  = "test_leveldb/"
			}'),
			{ class = "query", data = (named_t)"leveldb_rep", request = {
				action  = (action_t)"write",
				buffer  = (env_t)"buffer"
			}},
			{ class = "end" }
		}
	}
},
{ class = "end" }
