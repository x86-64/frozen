include(examples/article_cp_common.m4)

FILE(`remote_file1', `8888')
FILE(`remote_file2', `8887')

{ class => "daemon/thread" },
{ class => "emitter", request = {
	request = {
		action      = (action_t)"convert_to",
		source      = (machine_t)"remote_file1",
		destination = (machine_t)"remote_file2"
	},
	machine = (machine_t){
		{ class = "data/query", data = (env_t)"source" },
		{ class = "kill" }
	}
}},
{ class = "end" }
