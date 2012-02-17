include(shop.m4)
include(implode.m4)

// we are here
{ class = "thread" },
{ class = "emitter", request = {                        // write key to db
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

{ class = "emitter", request = {                        // read key from db
	request = {
		action = (action_t)"read",
		key    = (raw_t)"testkey",
		value  = (raw_t)"error\n"
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

{ class = "emitter", request = {                        // enumerate db
	request = {
		action = (action_t)"enum",
		data   = (zeromq_t){                    // our zeromq socket, we pass it to db, so it could use it
			type    = "push",
			connect = "tcp://127.0.0.1:8887",
			lazy    = (uint_t)"1"
		}
	},
	machine = (machine_t){
		SHOP_QUERY(`leveldb'),
		{ class = "assign", before = {
			myenum = (zeromq_t){            // this socket we use to get data
				type = "pull",
				bind = "tcp://127.0.0.1:8887"
			},
			buffer = (raw_t){}
		}},
		
		{ class = "modules/mustache", template = "Items list:\n{{#myenum}} - {{data}}\n{{/myenum}}", output = (hashkey_t)"buffer" },
		{ class = "data/query", data = (fd_t)"stdout", request = {
			action = (action_t)"write",
			buffer = (env_t)"buffer"
		}},
		{ class = "end" }
	}
}},
{ class = "kill" },
NULL,

// helper routines
SHOP(`leveldb',
`       IMPLODE(`input', `output',
	`{
		class  = "modules/zeromq",
		socket = (zeromq_t){
			type = "req",
			connect = "tcp://127.0.0.1:8888"
		},
		input  = (hashkey_t)"input",
		output = (hashkey_t)"output"
	}')
')

// -----------------------------------------------------------
// leveldb server

{ class = "thread", loop = (uint_t)"1" },
{
	class  = "modules/zeromq",
	socket = (zeromq_t){
		type = "rep",
		bind = "tcp://127.0.0.1:8888"
	},
	shop = (machine_t){
		EXPLODE(`buffer', `buffer',
		`{
			class = "modules/leveldb",
			path  = "test_leveldb/"
		}
		'),
		{ class = "shop/return" }
	}
},
{ class = "end" }

