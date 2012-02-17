include(implode.m4)

{ class = "thread", loop = (uint_t)"1" },
{
	class  = "modules/zeromq",
	socket = (zeromq_t){
		type = "rep",
		bind = "tcp://127.0.0.1:PORT"
	},
	shop = (machine_t){
		EXPLODE(`buffer', `buffer',
		`
			{ class = "query", data = (file_t){ filename = "FILE" } }
		'),
		{ class = "shop/return" }
	}
},
{ class = "end" }
