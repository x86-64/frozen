#define DATA_C
#include <libfrozen.h>
#include <dataproto.h>

       data_proto_t **         data_protos;
       uintmax_t               data_protos_nitems;
extern data_proto_t *          data_protos_static[];
extern size_t                  data_protos_static_size;

ssize_t              frozen_data_init(void){ // {{{
	if( (data_protos = malloc(data_protos_static_size)) == NULL)
		return -ENOMEM;
	
	memcpy(data_protos, &data_protos_static, data_protos_static_size);
	data_protos_nitems = data_protos_static_size / sizeof(data_proto_t *);
	return 0;
} // }}}
void                 frozen_data_destroy(void){ // {{{
	free(data_protos);
} // }}}
ssize_t              data_register(data_proto_t *proto){ // {{{
	uintmax_t              new_id            = data_protos_nitems;
	data_proto_t         **new_table;
	data_proto_t         **old_table;
	
	// alloc new table
	if( (new_table = malloc((new_id + 1) * sizeof(data_proto_t *))) == NULL)
		return -ENOMEM;
	
	// fill
	memcpy(new_table, data_protos, new_id * sizeof(data_proto_t *));
	new_table[new_id]  = proto;
	
	// swap tables
	old_table          = data_protos;
	data_protos        = new_table;
	data_protos_nitems = new_id + 1;
	free(old_table);
	printf("reg: %d %s\n", (int)new_id, proto->type_str);
	return 0;
} // }}}

ssize_t              data_query         (data_t *data, void *args){ // {{{
	f_data_func            func;
	data_proto_t          *proto;
	fastcall_header       *fargs             = (fastcall_header *)args;
	
	if(data == NULL || data->type >= data_protos_nitems)
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

