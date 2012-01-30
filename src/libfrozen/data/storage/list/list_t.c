#include <libfrozen.h>
#include <list_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>

static ssize_t data_list_t_alloc(data_t *data, fastcall_alloc *fargs){ // {{{
	if( (data->ptr = list_t_alloc()) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static ssize_t data_list_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
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
			
		default:
			break;
	};
	return -ENOSYS;
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

data_proto_t list_t_proto = {
	.type                   = TYPE_LISTT,
	.type_str               = "list_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_list_t_convert_from,
		[ACTION_ALLOC]        = (f_data_func)&data_list_t_alloc,
		[ACTION_FREE]         = (f_data_func)&data_list_t_free,
		[ACTION_PUSH]         = (f_data_func)&data_list_t_push,
		[ACTION_POP]          = (f_data_func)&data_list_t_pop,
	}
};

static list_chunk_t *    chunk_alloc(data_t *data){ // {{{
	list_chunk_t     *chunk;
	
	if(data == NULL)
		return NULL;
	
	if( (chunk = malloc(sizeof(list_chunk_t))) == NULL)
		return NULL;
	
	chunk->cnext       = NULL;
	memcpy(&chunk->data, data, sizeof(data_t));
	return chunk;
} // }}}
static void              chunk_free(list_chunk_t *chunk){ // {{{
	//fastcall_free r_free = { { 2, ACTION_FREE } };
	//data_query(&chunk->data, &r_free);
	
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
		
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&chunk->data, &r_free);
		
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
void            list_t_free                (list_t *list){ // {{{
	list_t_destroy(list);
	free(list);
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
