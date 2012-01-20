#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_convert data/convert
 */
/**
 * @ingroup mod_machine_convert
 * @page page_convert_info Description
 *
 * This module convert incoming data to any specified type if this possible
 */
/**
 * @ingroup mod_machine_convert
 * @page page_convert_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/convert",
 *              items                   = {
 *                      inputkey = {
 *                           type          = (datatype_t)'raw_t',       # desired type
 *                           output        = (hashkey_t)'buffer',       # output key, default eq inputkey
 *                           format        = (format_t)'clean',         # format to convert, default "clean"
 *                           convert_from  = (uint_t)'1'                # use convert_from routines instead, default 0 (use convert_to)     
 *                      },
 *                      ...
 *              }
 * }
 * @endcode
 */

#define EMODULE 36

typedef struct convert_userdata {
	hash_t                *items;
} convert_userdata;

typedef struct convert_ctx {
	machine_t             *machine;
	request_t             *request;
	request_t             *new_request;
} convert_ctx;

static int convert_init(machine_t *machine){ // {{{
	if((machine->userdata = calloc(1, sizeof(convert_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int convert_destroy(machine_t *machine){ // {{{
	convert_userdata        *userdata          = (convert_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int convert_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	convert_userdata      *userdata          = (convert_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHT,    userdata->items,        config, HK(items));
	return 0;
} // }}}

static ssize_t convert_iterator(hash_t *item, convert_ctx *ctx){ // {{{
	ssize_t                ret;
	hashkey_t              input_hk;
	hashkey_t              output_hk;
	datatype_t             type              = TYPE_INVALID;
	uintmax_t              format            = FORMAT(clean);
	uintmax_t              from              = 0;
	//uintmax_t              return_result     = 0;
	data_t                *input;
	hash_t                *new_hash;
	hash_t                *item_cfg          = (hash_t *)item->data.ptr;
	
	if(item->data.type != TYPE_HASHT)
		return ITER_CONTINUE;
	
	input_hk = output_hk = item->key;
	
	hash_data_get(ret, TYPE_DATATYPET, type,           item_cfg, HK(type));
	hash_data_get(ret, TYPE_UINTT,     format,         item_cfg, HK(format));
	hash_data_get(ret, TYPE_UINTT,     from,           item_cfg, HK(convert_from));
	hash_data_get(ret, TYPE_HASHKEYT,  output_hk,      item_cfg, HK(output));
	//hash_data_get(ret, TYPE_UINTT,     return_result,  item_cfg, HK(return));
	
	if( (new_hash = hash_new(3)) == NULL)
		return ITER_BREAK;
	
	new_hash[0].key       = input_hk;
	new_hash[0].data.type = type;
	new_hash[0].data.ptr  = NULL;
	hash_assign_hash_inline(&new_hash[1], ctx->new_request);
	
	if( (input = hash_data_find(ctx->request, input_hk)) == NULL)
		goto error;
	
	if(from == 0){
		fastcall_convert_to   r_convert = { { 4, ACTION_CONVERT_TO   }, &(new_hash[0].data), format };
		if(data_query(input, &r_convert) != 0)
			goto error;
	}else{
		fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, input, format };
		if(data_query(&(new_hash[0].data), &r_convert) != 0)
			goto error;
	}
	
	ctx->new_request = new_hash;
	return ITER_CONTINUE;
error:
	free(new_hash);
	return ITER_BREAK;
} // }}}

static ssize_t convert_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	convert_ctx            ctx               = { machine, request, hash_new(1) };
	convert_userdata      *userdata          = (convert_userdata *)machine->userdata;
	
	if(hash_iter(userdata->items, (hash_iterator)&convert_iterator, &ctx, 0) != ITER_OK){
		hash_free(ctx.new_request);
		return -EFAULT;
	}
	
	request_t r_next[] = {
		hash_inline(ctx.new_request),
		hash_inline(request),
		hash_end
	};
	ret = machine_pass(machine, r_next);
	
	hash_free(ctx.new_request);
	return ret;
} // }}}

machine_t convert_proto = {
	.class          = "data/convert",
	.supported_api  = API_HASH,
	.func_init      = &convert_init,
	.func_configure = &convert_configure,
	.func_destroy   = &convert_destroy,
	.machine_type_hash = {
		.func_handler = &convert_handler
	}
};

