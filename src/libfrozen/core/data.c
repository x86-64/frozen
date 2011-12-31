#define DATA_C
#include <libfrozen.h>

       data_proto_t **         data_protos;
       uintmax_t               data_protos_nitems;
extern data_proto_t *          data_protos_static[];
extern size_t                  data_protos_static_size;

static ssize_t data_default_donothing(data_t *data, void *hargs){ // {{{
	return -ENOSYS;
} // }}}
static void    data_fill_blanks(data_proto_t *p){ // {{{
	uintmax_t              j;
	f_data_func            func;
	
	switch(p->api_type){
		case API_DEFAULT_HANDLER:
			func = p->handler_default;
			for(j=0; j<sizeof(p->handlers)/sizeof(p->handlers[0]); j++)
				p->handlers[j] = func;
			break;
		case API_HANDLERS:
			for(j=0; j<sizeof(p->handlers)/sizeof(p->handlers[0]); j++){
				if(!p->handlers[j])
					p->handlers[j] = data_protos[ TYPE_DEFAULTT ]->handlers[j];
			}
			break;
	}
} // }}}

ssize_t              frozen_data_init(void){ // {{{
	uintmax_t              i;
	data_proto_t          *p;
	
	if( (data_protos = malloc(data_protos_static_size)) == NULL)
		return -ENOMEM;
	
	memcpy(data_protos, &data_protos_static, data_protos_static_size);
	data_protos_nitems = data_protos_static_size / sizeof(data_proto_t *);
	
	// prepare default_t
	for(i=0; i<sizeof(p->handlers)/sizeof(p->handlers[0]); i++){
		if(!data_protos[ TYPE_DEFAULTT ]->handlers[i])
			data_protos[ TYPE_DEFAULTT ]->handlers[i] = &data_default_donothing;
	}
	
	// prepare rest
	for(i=0; i<data_protos_nitems; i++){
		if(i == TYPE_DEFAULTT)
			continue;
		
		data_fill_blanks(data_protos[i]);
	}
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
	
	// fill blank handlers
	data_fill_blanks(proto);
	
	// swap tables
	old_table          = data_protos;
	data_protos        = new_table;
	data_protos_nitems = new_id + 1;
	free(old_table);
	return 0;
} // }}}

ssize_t              data_query         (data_t *data, void *args){ // {{{
	register fastcall_header       *fargs             = (fastcall_header *)args;
	
	if(data == NULL || data->type >= data_protos_nitems || fargs->action >= ACTION_INVALID)
		return -EINVAL;
	
	return data_protos[ data->type ]->handlers[ fargs->action ](data, args);
} // }}}

