#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_convert data/convert
 */
/**
 * @ingroup mod_backend_convert
 * @page page_convert_info Description
 *
 * This module convert incoming data to any specified type if this possible
 */
/**
 * @ingroup mod_backend_convert
 * @page page_convert_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/convert",
 *              items                   = {
 *                      {
 *                           input         = (hashkey_t)'buffer',       # input key
 *                           type          = (datatype_t)'raw_t',       # desired type
 *                           format        = (format_t)'clean'          # format to convert, default "clean"
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
	backend_t             *backend;
	request_t             *request;
	request_t             *new_request;
} convert_ctx;

static int convert_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(convert_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int convert_destroy(backend_t *backend){ // {{{
	convert_userdata        *userdata          = (convert_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int convert_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	convert_userdata      *userdata          = (convert_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT,    userdata->items,        config, HK(items));
	return 0;
} // }}}

static ssize_t convert_iterator(hash_t *item, convert_ctx *ctx){ // {{{
	ssize_t                ret;
	hashkey_t             input;
	datatype_t             type              = TYPE_INVALID;
	uintmax_t              format            = FORMAT(clean);
	//uintmax_t              return_result     = 0;
	data_t                *data;
	hash_t                *new_hash;
	hash_t                *item_cfg          = (hash_t *)item->data.ptr;
	
	if(item->data.type != TYPE_HASHT)
		return ITER_CONTINUE;
	
	hash_data_copy(ret, TYPE_DATATYPET, type,           item_cfg, HK(type));
	hash_data_copy(ret, TYPE_UINTT,     format,         item_cfg, HK(format));
	//hash_data_copy(ret, TYPE_UINTT,     return_result,  item_cfg, HK(return));
	hash_data_copy(ret, TYPE_HASHKEYT,  input,          item_cfg, HK(input));
	if(ret != 0)
		return ITER_BREAK;
	
	if( (new_hash = hash_new(3)) == NULL)
		return ITER_BREAK;
	
	new_hash[0].key       = input;
	new_hash[0].data.type = type;
	new_hash[0].data.ptr  = NULL;
	hash_assign_hash_inline(&new_hash[1], ctx->new_request);
	
	if( (data = hash_data_find(ctx->request, input)) == NULL)
		goto error;
	
	fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, data, format };
	if(data_query(&(new_hash[0].data), &r_convert) != 0)
		goto error;
	
	ctx->new_request = new_hash;
	return ITER_CONTINUE;
error:
	free(new_hash);
	return ITER_BREAK;
} // }}}

static ssize_t convert_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	convert_ctx            ctx               = { backend, request, hash_new(1) };
	convert_userdata      *userdata          = (convert_userdata *)backend->userdata;
	
	if(hash_iter(userdata->items, (hash_iterator)&convert_iterator, &ctx, 0) != ITER_OK){
		hash_free(ctx.new_request);
		return -EFAULT;
	}
	
	request_t r_next[] = {
		hash_inline(ctx.new_request),
		hash_inline(request),
		hash_end
	};
	ret = ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
	
	hash_free(ctx.new_request);
	return ret;
} // }}}

backend_t convert_proto = {
	.class          = "data/convert",
	.supported_api  = API_HASH,
	.func_init      = &convert_init,
	.func_configure = &convert_configure,
	.func_destroy   = &convert_destroy,
	.backend_type_hash = {
		.func_handler = &convert_handler
	}
};

