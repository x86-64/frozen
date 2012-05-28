include(shop.m4)
include(implode.m4)

define(`FILE',`
SHOP($1,
`       
	IMPLODE(`input', `output',
	`{
		class  = "modules/zeromq",
		socket = (zeromq_t){
			type = "req",
			connect = "tcp://127.0.0.1:$2"
		},
		input  = (hashkey_t)"input",
		output = (hashkey_t)"output"
	}'),
	{ class = "end", return = (env_t)"ret" }
')
')
