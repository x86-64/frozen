define(`IMPLODE',
`{ class = "try", shop = (machine_t){
	{ class = "implode" },
	
	{ class = "assign", before = { packed = (raw_t)"" }, copy = (uint_t)"1" },
	{ class = "query", data = (env_t)"buffer", request = {
		action      = (action_t)"convert_to",
		destination = (env_t)"$1",
		format      = (format_t)"binary"
	}},
	
	$2,
	
	{ class = "query", data = (env_t)"buffer", request = {
		action      = (action_t)"convert_from",
		source      = (env_t)"$1",
		format      = (format_t)"binary"
	}},
	{ class = "shop/return" }
}}')dnl
define(`EXPLODE',
`{ class = "try", request = (hashkey_t)"$1", request_out = (hashkey_t)"packed", shop = (machine_t){
	$2,
	{ class = "shop/return" }
}},
{ class = "query", data = (env_t)"$1", request = { action = (action_t)"free" } },
{ class = "assign", before = { $1 = (raw_t)"" }, copy = (uint_t)"1" },
{ class = "query", data = (env_t)"packed", request = {
	action      = (action_t)"convert_to",
	destination = (env_t)"$1",
	format      = (format_t)"binary"
}}
')dnl
