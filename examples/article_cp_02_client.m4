include(examples/article_cp_common.m4)

FILE(`remote_file', `8888')


{ class => "emitter", request = {
	request = {
		action      = (action_t)"convert_to",
		source      = (file_t){ filename = "INPUT" },
		destination = (machine_t)"remote_file"
	},
	machine = (machine_t){
		{ class = "data/query", data = (env_t)"source" },
		{ class = "end" }
	}
}},
{ class = "end" }
