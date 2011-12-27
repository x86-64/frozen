#define DATA_C
#include <libfrozen.h>
#include <dataproto.h>

ssize_t              data_query         (data_t *data, void *args){ // {{{
	f_data_func            func;
	data_proto_t          *proto;
	fastcall_header       *fargs             = (fastcall_header *)args;
	
	if(data == NULL || (unsigned)data->type >= data_protos_size)
		return -EINVAL;
	
	proto = data_protos[data->type];
	
	switch(proto->api_type){
		case API_DEFAULT_HANDLER: func = proto->handler_default;           break;
		case API_HANDLERS:        func = proto->handlers[ fargs->action ]; break;
	};
	if( func == NULL && (func = data_protos[TYPE_DEFAULTT]->handlers[ fargs->action ]) == NULL)
		return -ENOSYS;
	
	return func(data, args);
} // }}}

