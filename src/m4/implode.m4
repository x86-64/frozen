define(`IMPLODE', dnl IMPLODE(input, output, func)
`{ class = "try", shop = (machine_t){
	{ class = "implode", buffer = (hashkey_t)"$1" },
	
	$3,
	
	{ class = "query", data = (env_t)"$1", request = {
		action      = (action_t)"convert_from",
		source      = (env_t)"$2",
		format      = (format_t)"binary"
	}},
	{ class = "shop/return" }
}}')dnl
define(`EXPLODE', dnl EXPLODE(input, output, func)
`{ class = "try", request = (hashkey_t)"$1", request_out = (hashkey_t)"$2", shop = (machine_t){
	$3,
	{ class = "shop/return" }
}}
')dnl
