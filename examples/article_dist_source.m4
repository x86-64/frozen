{ class = "thread" },
{ class = "query", data = (file_t){ filename = "/var/log/messages", readonly = (uint_t)"1" }, request = {
	action      = (action_t)"convert_to",
	destination = (machine_t){
		
		// split file chunks to lines
		{ class = "split" },
		
		// send to next process
		{ class = "modules/zeromq", convert = (uint_t)"1", socket = (zeromq_t){
			type    = "push",
			connect = "tcp://127.0.0.1:8800"
		}},
		{ class = "end" }
	}
}},
{ class = "kill" }
