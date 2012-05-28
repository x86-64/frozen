{ class = "thread", loop = (uint_t)"1" },
{ class = "modules/zeromq", socket = (zeromq_t){
	type = "pull",
	bind = "tcp://127.0.0.1:8801"
}},
{ class = "query",
	data = (slider_t){
		data = (file_t){ filename = "result.dat" }
	},
	request = {
		action = (action_t)"write",
		buffer = (env_t)"buffer"
	}
},
{ class = "end" }

{ class = "delay", sleep = (uint_t)"10000" },
{ class = "end" }
