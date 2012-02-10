define(`IMPLODE',
`{ class = "try", shop = (machine_t){
	{ class = "assign", before = { return_to = (void_t)"" } },
	{ class = "implode" },
	
	{ class = "assign", before = { packed = (raw_t)"" }},
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
	{ class = "end" }
}}')dnl
define(`EXPLODE',
`{ class = "assign", before = { request = (hash_t){} }},
{ class = "query", data = (env_t)"request", request = {
	action      = (action_t)"convert_from",
	source      = (env_t)"$1",
	format      = (format_t)"binary"
}},
{ class = "explode", buffer = (hashkey_t)"request" },

$2,

{ class = "implode", buffer = (hashkey_t)"request" },

{ class = "assign", before = { buffer = (raw_t)"" }},
{ class = "query", data = (env_t)"request", request = {
	action      = (action_t)"convert_to",
	destination = (env_t)"$1",
	format      = (format_t)"binary"
}}')dnl
