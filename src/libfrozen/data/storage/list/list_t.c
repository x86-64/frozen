#include <libfrozen.h>
#include <list_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <modifiers/slider/slider_t.h>
#include <special/ref/ref_t.h>
#include <core/data/data_t.h>
#include <numeric/uint/uint_t.h>
#include <special/consumable/consumable_t.h>

typedef struct ctx_lookup {
	uintmax_t              key_uint;
	data_t                 output;
} ctx_lookup;

ssize_t        data_list_t(data_t *data){ // {{{
	if( (data->ptr = list_t_alloc()) == NULL)
		return -ENOMEM;
	
	data->type = TYPE_LISTT;
	return 0;
} // }}}

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
static ssize_t iter_list_t_lookup(data_t *data, ctx_lookup *ctx){ // {{{
	if(ctx->key_uint-- == 0){
		ctx->output = *data;
		return 1;
	}
	return 0;
} // }}}
static ssize_t iter_list_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_copy;
	
	holder_copy(ret, &d_copy, data);
	if(ret < 0)
		return ret;
	
	fastcall_push r_push = { { 3, ACTION_PUSH }, &d_copy };
	return data_query(fargs->dest, &r_push);
} // }}}
static ssize_t iter_list_t_convert_from(hash_t *hash_item, data_t *ctx){ // {{{
	ssize_t                ret;
	list_t                *fdata             = (list_t *)ctx->ptr;
	
	ret = list_t_push(fdata, &hash_item->data);
	data_set_void(&hash_item->data);
	
	return (ret < 0) ? ITER_BREAK : ITER_CONTINUE;
} // }}}

static ssize_t data_list_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret               = 0;
	ssize_t                qret;
	list_chunk_t          *chunk;
	list_t                *fdata             = (list_t *)src->ptr;
	data_t                 sl_output         = DATA_SLIDERT(fargs->dest, 0);
	
	switch(fargs->format){
		case FORMAT(packed):;
			for(chunk = fdata->head; chunk; chunk = chunk->cnext){
				// remove ref_t from top
				hash_t           r_key[] = {
					{ 0, DATA_HASHKEYT(HK(data)) },
					hash_end
				};
				data_t           d_key     = DATA_PTR_HASHT(r_key);
				fastcall_control r_control = {
					{ 5, ACTION_CONTROL },
					HK(data),
					&d_key,
					NULL
				};
				if( (ret = data_query(&chunk->data, &r_control)) < 0)
					break;
				
				data_t         d_data             = DATA_PTR_DATAT(r_control.value);
				
				fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &sl_output, FORMAT(packed) };
				ret = data_query(&d_data, &r_convert);
				
				data_slider_t_set_offset(&sl_output, r_convert.transfered, SEEK_CUR);
				
				if(ret < 0)
					break;
			}
			
			// terminate list
			data_t                 terminator         = { TYPE_LISTENDT, NULL };
			data_t                 d_data             = DATA_PTR_DATAT(&terminator);
			
			fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &sl_output, FORMAT(packed) };
			qret = data_query(&d_data, &r_convert);
			
			data_slider_t_set_offset(&sl_output, r_convert.transfered, SEEK_CUR);
			
			if(qret < 0)
				ret = qret;
			break;
			
		default:
			return -ENOSYS;
	}
	if(fargs->header.nargs >= 5)
		fargs->transfered = data_slider_t_get_offset(&sl_output);
	
	return ret;
} // }}}
static ssize_t data_list_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret               = 0;
	list_t                *fdata;
	uintmax_t              transfered        = 0;
	data_t                 sl_input          = DATA_SLIDERT(fargs->src, 0);
	
	switch(fargs->format){
		case FORMAT(native):
		case FORMAT(human):
		case FORMAT(config):;
			return data_list_t(dst);
			
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if((fdata = dst->ptr) == NULL){
				if( (fdata = dst->ptr = list_t_alloc()) == NULL)
					return -ENOMEM;
			}

			if(hash_iter(config, (hash_iterator)&iter_list_t_convert_from, dst, 0) != ITER_OK){
				list_t_free(dst->ptr);
				return -EFAULT;
			}
			return 0;
			
		case FORMAT(packed):;
			data_t                 item;
			data_t                 d_item    = DATA_PTR_DATAT(&item);
			
			if((fdata = dst->ptr) == NULL){
				if( (fdata = dst->ptr = list_t_alloc()) == NULL)
					return -ENOMEM;
			}
			
			while(1){
				data_set_void(&item);
				
				fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &sl_input, FORMAT(packed) };
				if( (ret = data_query(&d_item, &r_convert)) < 0)
					break;
				
				data_slider_t_set_offset(&sl_input, r_convert.transfered, SEEK_CUR);
				
				if(item.type == TYPE_LISTENDT)
					break;
				
				if( (ret = list_t_push(fdata, &item)) < 0)
					break;
			}
			transfered = data_slider_t_get_offset(&sl_input);
			break;

		default:
			return -ENOSYS;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_list_t_free(data_t *data, fastcall_free *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	list_t_free(fdata);
	return 0;
} // }}}
static ssize_t data_list_t_create(data_t *data, fastcall_crud *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fargs->value == NULL) // EOF
		return 0;
	
	if(fdata == NULL){
		if( (fdata = data->ptr = list_t_alloc()) == NULL)
			return -ENOMEM;
	}
	
	if(fargs->key == NULL || data_is_void(fargs->key))
		return list_t_push(fdata, fargs->value);
	
	return -ENOSYS;
} // }}}
static ssize_t data_list_t_lookup(data_t *data, fastcall_lookup *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              key_uint;
	data_t                 d_key;
	data_t                *d_key_ptr         = &d_key;
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fargs->key == NULL || fargs->value == NULL)
		return -EINVAL;
	
	if(helper_key_current(fargs->key, &d_key) < 0)
		d_key_ptr = fargs->key;
	
	data_get(ret, TYPE_UINTT, key_uint, d_key_ptr);
	if(ret == 0){
		ctx_lookup ctx = { key_uint };
		if(list_t_enum(fdata, (list_t_callback)&iter_list_t_lookup, &ctx) != 0)
			return data_notconsumable_t(fargs->value, ctx.output);
	}
	return -EINVAL;
} // }}}
static ssize_t data_list_t_delete(data_t *data, fastcall_crud *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	if(fargs->key == NULL)
		return list_t_pop(fdata, fargs->value);
	
	return -ENOSYS;
} // }}}
static ssize_t data_list_t_push(data_t *data, fastcall_push *fargs){ // {{{
	list_t                *fdata             = (list_t *)data->ptr;
	
	if(fargs->data == NULL) // EOF
		return 0;
	
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
	ssize_t                ret;
	list_t                *fdata             = (list_t *)data->ptr;
	
	ret = list_t_enum(fdata, (list_t_callback)&iter_list_t_enum, fargs);
	
	fastcall_push r_push = { { 3, ACTION_PUSH }, NULL }; // EOF
	data_query(fargs->dest, &r_push);
	
	return ret;
} // }}}
data_proto_t list_t_proto = { // {{{
	.type                   = TYPE_LISTT,
	.type_str               = "list_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_list_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_list_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_list_t_free,
		[ACTION_COMPARE]      = (f_data_func)&data_list_t_compare,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
		
		[ACTION_CREATE]       = (f_data_func)&data_list_t_create,
		[ACTION_LOOKUP]       = (f_data_func)&data_list_t_lookup,
		[ACTION_DELETE]       = (f_data_func)&data_list_t_delete,
		[ACTION_ENUM]         = (f_data_func)&data_list_t_enum,
		[ACTION_PUSH]         = (f_data_func)&data_list_t_push,
		[ACTION_POP]          = (f_data_func)&data_list_t_pop,
	}
}; // }}}

static ssize_t data_list_end_t_nosys(data_t *data, fastcall_header *hargs){ // {{{
	return -ENOSYS;
} // }}}
static ssize_t data_list_end_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	return 0;
} // }}}
static ssize_t data_list_end_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	return 0;
} // }}}
data_proto_t list_end_t_proto = { // {{{
	.type                   = TYPE_LISTENDT,
	.type_str               = "list_end_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_list_end_t_nosys,
	.handlers               = {
		[ACTION_CONVERT_TO]    = (f_data_func)&data_list_end_t_convert_to,
		[ACTION_CONVERT_FROM]  = (f_data_func)&data_list_end_t_convert_from,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
}; // }}}

static list_chunk_t *    chunk_alloc(data_t *data){ // {{{
	ssize_t                ret;
	list_chunk_t          *chunk;
	data_t                 consumed;
	
	holder_consume(ret, consumed, data);
	if(ret < 0)
		return NULL;
	
	ref_t                 *ref               = ref_t_alloc(&consumed);
	data_t                 d_ref             = DATA_PTR_REFT(ref);
	
	if(data == NULL || ref == NULL)
		return NULL;
	
	if( (chunk = malloc(sizeof(list_chunk_t))) == NULL)
		return NULL;
	
	chunk->cnext       = NULL;
	chunk->data        = d_ref;
	return chunk;
} // }}}
static void              chunk_free(list_chunk_t *chunk){ // {{{
	data_free(&chunk->data);
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
	ssize_t                ret;
	list_t                *new_list;
	list_chunk_t          *chunk;
	data_t                 data_copy;
	
	new_list = list_t_alloc();
	for(chunk = list->head; chunk; chunk = chunk->cnext){
		holder_copy(ret, &data_copy, &chunk->data);
		if(ret < 0)
			goto error;
		
		list_t_push(new_list, &data_copy);
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
ssize_t         list_t_shift               (list_t *list, data_t *data){ // {{{
	ssize_t           ret;
	data_t            d_copy;
	list_chunk_t     *chunk;
	
	if(list->head == NULL)
		return -EINVAL;
	
	chunk      = list->head;
	list->head = chunk->cnext;
	
	//if(list->tail == chunk)
	//	list->tail = NULL;
	
	holder_copy(ret, &d_copy, &chunk->data);
	if(ret < 0)
		return ret;
	
	*data = d_copy;
	chunk_free(chunk);
	return 0;
} // }}}
ssize_t         list_t_push                (list_t *list, data_t *data){ // {{{
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
ssize_t         list_t_pop                 (list_t *list, data_t *data){ // {{{
	ssize_t           ret;
	data_t            d_copy;
	list_chunk_t     *curr;
	list_chunk_t     **prev                  = &list->head;
	
	for(curr = list->head; curr; curr = curr->cnext){
		if(curr->cnext == NULL){
			*prev = NULL;
			
			holder_copy(ret, &d_copy, &curr->data);
			if(ret < 0)
				return ret;
			
			*data = d_copy;
			chunk_free(curr);
			return 0;
		}
		prev = &curr->cnext;
	}
	return -EINVAL;
} // }}}

ssize_t         list_t_enum                (list_t *list, list_t_callback callback, void *userdata){ // {{{
	ssize_t           ret;
	list_chunk_t     *curr;
	
	if(list == NULL)
		return -EINVAL;
	
	for(curr = list->head; curr; curr = curr->cnext){
		if( (ret = callback(&curr->data, userdata)) != 0)
			return ret;
	}
	return 0;
} // }}}


