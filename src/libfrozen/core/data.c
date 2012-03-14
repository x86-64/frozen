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
	
	p->type_port = portable_hash(p->type_str);
	switch(p->api_type){
		case API_DEFAULT_HANDLER:
			func = p->handler_default;
			for(j=0; j<sizeof(p->handlers)/sizeof(p->handlers[0]); j++)
				p->handlers[j] = func;
			break;
		case API_HANDLERS:
			for(j=0; j<sizeof(p->handlers)/sizeof(p->handlers[0]); j++){
				if(!p->handlers[j]){
					if(p->handler_default){
						p->handlers[j] = p->handler_default;
					}else{
						p->handlers[j] = data_protos[ TYPE_DEFAULTT ]->handlers[j];
					}
				}
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

/*ssize_t              data_query         (data_t *data, void *args){ // {{{
	register fastcall_header       *fargs             = (fastcall_header *)args;
	
	if(data == NULL || data->type >= data_protos_nitems || fargs->action >= ACTION_INVALID)
		return -EINVAL;
	
	return data_protos[ data->type ]->handlers[ fargs->action ](data, args);
} // }}}*/

ssize_t              data_get_continious(data_t *data, data_t *freeme, void **ptr, uintmax_t *ptr_size){ // {{{
	ssize_t                ret;
	
	// direct pointers
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	fastcall_length      r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(native) };
	if( data_query(data, &r_len) == 0 && data_query(data, &r_ptr) == 0 && r_ptr.ptr != NULL){
		data_set_void(freeme);
		ret = 0;
		goto ok;
	}
	
	freeme->type = TYPE_RAWT;
	freeme->ptr  = NULL;
	
	// strange types
	fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, freeme, FORMAT(native) };
	if( (ret = data_query(data, &r_convert)) < 0)
		return ret;
	
	if( data_query(freeme, &r_len) == 0 && data_query(freeme, &r_ptr) == 0 && r_ptr.ptr != NULL){
		ret = 1;
		goto ok;
	}
	
	return -EFAULT;
ok:
	*ptr      = r_ptr.ptr;
	*ptr_size = r_len.length;
	return ret;
} // }}}

ssize_t              data_make_flat(data_t *data, format_t format, data_t *freeme, void **ptr, uintmax_t *ptr_size){ // {{{
	ssize_t                ret;
	data_t                 temp;
	
	// native format speed-up
	if(format == FORMAT(native)){
		// try direct pointers
		fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
		fastcall_length      r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(native) };
		if( data_query(data, &r_len) == 0 && data_query(data, &r_ptr) == 0){
			if(r_ptr.ptr == NULL && r_len.length != 0)
				return -EFAULT;
			
			data_set_void(freeme);
			*ptr      = r_ptr.ptr;
			*ptr_size = r_len.length;
			return 0;
		}
	}
	
	temp.type = TYPE_RAWT;
	temp.ptr  = NULL;
	
	fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &temp, format };
	if( (ret = data_query(data, &r_convert)) < 0)
		return ret;
	
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	fastcall_length      r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(native) };
	if( data_query(&temp, &r_len) == 0 && data_query(&temp, &r_ptr) == 0){
		if(r_ptr.ptr == NULL && r_len.length != 0)
			return -EFAULT;
		
		*freeme   = temp;
		*ptr      = r_ptr.ptr;
		*ptr_size = r_len.length;
		return 1;
	}
	return -EFAULT;
} // }}}

