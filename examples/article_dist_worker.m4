{ class = "thread", loop = (uint_t)"1" },

// get message
{ class = "modules/zeromq", socket = (zeromq_t){
	type    = "pull",
	bind    = "tcp://127.0.0.1:8800"
}},

// print to console
{ class = "query", data = (fd_t)"stdout", request = { action = (action_t)"write", buffer = (env_t)"buffer" } },

// send to next process
{ class = "modules/zeromq", socket = (zeromq_t){
	type    = "push",
	connect = "tcp://127.0.0.1:8801"
}},
{ class = "end" }
