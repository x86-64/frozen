{ class => "daemon/thread" },
{ class => "emitter", request = {
	request = {
		action      = (action_t)"convert_to",
		source      = (file_t){ filename = "INPUT"  },
		destination = (file_t){ filename = "OUTPUT" }
	},
	machine = (machine_t){
		{ class = "data/query", data = (env_t)"source" },
		{ class = "kill" }
	}
},
{ class = "end" }
