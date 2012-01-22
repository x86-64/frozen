{ class => "daemon/thread" },
{ class => "request/emitter",
	request = {
		request = { buffer = (raw_t) { length = (uint_t)'50', buffer = "HELLO world!\n" }, worldme = "moooo!" },
		machine = (machine_t){
			{ class => "io/stdout" }
		}
	}
},
{ class => "daemon/kill" }
