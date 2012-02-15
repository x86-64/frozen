#include <libfrozen.h>
#include <list_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <modifiers/slider/slider_t.h>
#include <special/ref/ref_t.h>

#define DATATYPE_BIN uint16_t

static ssize_t iter_list_t_compare(data_t *data, fastcall_compare *fargs){ // {{{
	ssize_t                ret;
	
	fastcall_compare r_compare = { { 4, ACTION_COMPARE }, fargs->data2 };
	if( (ret = data_query(data, &r_compare)) < 0)
		return ret;
	
	if(r_compare.result == 0){ // if equal - exit
		fargs->result = 0;
		return 1;
	}
	return 0;
} // }}}
static ssize_t iter_list_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	fastcall_acquire r_acquire = { { 2, ACTION_ACQUIRE } };
	data_query(data, &r_acquire);
	
	fastcall_push r_push = { { 3, ACTION_PUSH }, data };
	return data_query(fargs->dest, &r_push);
} // }}}

static ssize_t data_list_t_alloc(data_t *data, fastcall_alloc *fargs){ // {{{
	if( (data->ptr = list_t_alloc()) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static ssize_t data_list_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret               = 0;
	ssize_t                qret;
	list_chunk_t          *chunk;
	list_t                *fdata             = (list_t *)src->ptr;
	data_t                 d_sl_data         = DATA_SLIDERT(fargs->dest, 0);
	
	switch(fargs->format){
		case FORMAT(binary):;
			DATATYPE_BIN           type;
			
			for(chunk = fdata->head; chunk; chunk = chunk->cnext){
				// strip refs
				fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
				if( (ret = data_query(&chunk->data, &r_getdata)) < 0)
					break;
				
				type = (DATATYPE_BIN)r_getdata.data->type;
				
				// write type
				fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &type, sizeof(type) };
				if( (ret = data_query(&d_sl_data, &r_write)) < 0)
					break;
				
				data_slider_t_freeze(&d_sl_data);
					
					// write data
					fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, &d_sl_data, FORMAT(binary) };
					if( (ret = data_query(&chunk->data, &r_convert)) < 0)
						break;
				
				data_slider_t_unfreeze(&d_sl_data);
			}
			
			// terminate list
			type = __MAX(DATATYPE_BIN);
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &type, sizeof(type) };
			if( (qret = data_query(&d_sl_data, &r_write)) < 0)
				ret = qret;
			break;
			
		default:
			return -ENOSYS;
	}
	return ret;
} // }}}
static ssize_t data_list_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret               = 0;
	list_t                *fdata;
	data_t                 d_sl_data         = DATA_SLIDERT(fargs->src, 0);
	
	switch(fargs->format){
		case FORMAT(clean):
		case FORMAT(human):
		case FORMAT(config):;
			return data_list_t_alloc(dst, NULL);
			
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return data_list_t_alloc(dst, NULL);
			
		case FORMAT(binary):;
			DATATYPE_BIN           type;
			data_t                 item;
			
			if((fdata = dst->ptr) == NULL){
				if( (fdata = dst->ptr = list_t_alloc()) == NULL)
					return -ENOMEM;
			}
			
			while(1){
				// read type
				fastcall_read r_read = { { 5, ACTION_READ }, 0, &type, sizeof(type) };
				if( (ret = data_query(&d_sl_data, &r_read)) < 0)
					break;
				
				if(type == __MAX(DATATYPE_BIN))
					break;
				
				item.type = type;
				item.ptr  = NULL;
				
				data_slider_t_freeze(&d_sl_data);
					
					// read data
					fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_sl_data, FORMAT(binary) };
					if( (ret = data_query(&item, &r_convert)) < 0)
						break;
				
				data_slider_t_unfreeze(&d_sl_data);
				
				if( (ret = list_t_unshift(fdata, &item)) < 0)
					break;
			}
			break;

		default:
			return -ENOSYS;
	};
	return ret;
} // }}}
static ssize_t data_list_t_free(data_t *data, fastcall_free *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	list_t_free(fdata);
	return 0;
} // }}}
static ssize_t data_list_t_push(data_t *data, fastcall_push *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fdata == NULL){
		if( (fdata = data->ptr = list_t_alloc()) == NULL)
			return -ENOMEM;
	}
	
	return list_t_push(fdata, fargs->data);
} // }}}
static ssize_t data_list_t_pop(data_t *data, fastcall_pop *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	return list_t_pop(fdata, fargs->data);
} // }}}
static ssize_t data_list_t_copy(data_t *data, fastcall_copy *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	if( (fargs->dest->ptr = list_t_copy(fdata)) == NULL)
		return -EFAULT;
	
	fargs->dest->type = data->type;
	return 0;
} // }}}
static ssize_t data_list_t_compare(data_t *data1, fastcall_compare *fargs){ // {{{
	list_t                *fdata             = (list_t *)data1->ptr;
	
	if(fargs->data2->type != TYPE_LISTT){
		// compare item with list, behave like list is multiselect list
		fargs->result = 2; // not equal, data2 is less
		return list_t_enum(fdata, (list_t_callback)&iter_list_t_compare, fargs);
	}
	
	// compare lists
	return -ENOSYS;
} // }}}
static ssize_t data_list_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	return list_t_enum(fdata, (list_t_callback)&iter_list_t_enum, fargs);
} // }}}

data_proto_t list_t_proto = {
	.type                   = TYPE_LISTT,
	.type_str               = "list_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_list_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_list_t_convert_from,
		[ACTION_ALLOC]        = (f_data_func)&data_list_t_alloc,
		[ACTION_FREE]         = (f_data_func)&data_list_t_free,
		[ACTION_PUSH]         = (f_data_func)&data_list_t_push,
		[ACTION_POP]          = (f_data_func)&data_list_t_pop,
		[ACTION_COPY]         = (f_data_func)&data_list_t_copy,
		[ACTION_COMPARE]      = (f_data_func)&data_list_t_compare,
		[ACTION_ENUM]         = (f_data_func)&data_list_t_enum,
	}
};

static list_chunk_t *    chunk_alloc(data_t *data){ // {{{
	list_chunk_t          *chunk;
	ref_t                 *ref               = ref_t_alloc(data);
	data_t                 d_ref             = DATA_PTR_REFT(ref);
	
	if(data == NULL || ref == NULL)
		return NULL;
	
	if( (chunk = malloc(sizeof(list_chunk_t))) == NULL)
		return NULL;
	
	chunk->cnext       = NULL;
	memcpy(&chunk->data, &d_ref, sizeof(data_t));
	return chunk;
} // }}}
static void              chunk_free(list_chunk_t *chunk){ // {{{
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&chunk->data, &r_free);
	
	free(chunk);
} // }}}

void            list_t_init                (list_t *list){ // {{{
	list->head  = NULL;
	//list->tail  = NULL;
} // }}}
void            list_t_destroy             (list_t *list){ // {{{
	list_chunk_t          *chunk;
	list_chunk_t          *next;
	
	for(chunk = list->head; chunk; chunk = next){
		next = chunk->cnext;
		
		chunk_free(chunk);
	}
	list->head = NULL;
} // }}}

list_t *        list_t_alloc               (void){ // {{{
	list_t           *list;

	if( (list = malloc(sizeof(list_t))) == NULL)
		return NULL;
	
	list_t_init(list);
	return list;
} // }}}
list_t *        list_t_copy                (list_t *list){ // {{{
	list_t                *new_list;
	list_chunk_t          *chunk;
	data_t                 data_copy;
	
	new_list = list_t_alloc();
	for(chunk = list->head; chunk; chunk = chunk->cnext){
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &data_copy };
		if(data_query(&chunk->data, &r_copy) < 0)
			goto error;
		
		list_t_unshift(new_list, &data_copy);
	}
	return new_list;
error:
	list_t_free(new_list);
	return NULL;
} // }}}
void            list_t_free                (list_t *list){ // {{{
	list_t_destroy(list);
	free(list);
} // }}}

ssize_t         list_t_unshift             (list_t *list, data_t *data){ // {{{
	list_chunk_t     *chunk;
	list_chunk_t     *curr;
	
	if( (chunk = chunk_alloc(data)) == NULL)
		return -ENOMEM;
	
	for(curr = list->head; curr; curr = curr->cnext){
		if(curr->cnext == NULL){
			curr->cnext = chunk;
			return 0;
		}
	}
	list->head = chunk;
	return 0;
} // }}}
ssize_t         list_t_push                (list_t *list, data_t *data){ // {{{
	list_chunk_t     *chunk;
	
	if( (chunk = chunk_alloc(data)) == NULL)
		return -ENOMEM;
	
	//if(list->head == NULL){
	//	list->tail = chunk;
	//}else{
		chunk->cnext = list->head;
	//}
	list->head = chunk;
	return 0;
} // }}}
ssize_t         list_t_pop                 (list_t *list, data_t *data){ // {{{
	list_chunk_t     *chunk;
	
	if(list->head == NULL)
		return -EINVAL;
	
	chunk      = list->head;
	list->head = chunk->cnext;
	
	//if(list->tail == chunk)
	//	list->tail = NULL;
	
	memcpy(data, &chunk->data, sizeof(data_t));
	chunk_free(chunk);
	return 0;
} // }}}

ssize_t         list_t_enum                (list_t *list, list_t_callback callback, void *userdata){ // {{{
	ssize_t           ret;
	list_chunk_t     *curr;
	
	for(curr = list->head; curr; curr = curr->cnext){
		if( (ret = callback(&curr->data, userdata)) != 0)
			return ret;
	}
	return 0;
} // }}}


