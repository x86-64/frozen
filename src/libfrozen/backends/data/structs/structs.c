#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_structs data/structs
 */
/**
 * @ingroup mod_backend_structs
 * @page page_structs_info Description
 *
 * This backend pack and unpack user requests accoring defined structure. It pack values as it is, to convert use @ref mod_backend_convert.
 */
/**
 * @ingroup mod_backend_structs
 * @page page_structs_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/structs",
 *              buffer                  = (hashkey_t)'buffer', # input/output request key name, default "buffer"
 *              values                  = (hashkey_t)'some',   # if supplied - request key name to take/write values from/to, default take from request
 *              size                    = (hashkey_t)'size',   # if supplied - add size of packed structure to request with key
 *              lazy                    = (uint_t)'1',         # use lazy packing (no size would be avaliable), default 0
 *              structure               = {                     # structure to pack/unpack to/from
 *                     keyname = {                              # - first field is keyname with default value of 10 and FORMAT(clean)
 *                                  default = (uint_t)'10',
 *                                  format  = (format_t)'clean'
 *                               }
 *                     key2    = { },                           # - second field is anything without default value and FORMAT(binary)
 *                     ...
 *              }
 * }
 * @endcode
 */

#define EMODULE 5

typedef enum struct_values {
	STRUCT_VALUES_WHOLE,
	STRUCT_VALUES_ONE
} struct_values;

typedef struct struct_userdata {
	hash_t       *structure;
	hashkey_t     buffer;
	hashkey_t     key_values;
	hashkey_t     size;
	struct_values  values;
	uintmax_t              lazy;
} struct_userdata;

static int struct_init(backend_t *backend){ // {{{
	struct_userdata       *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(struct_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->buffer = HK(buffer);
	
	return 0;
} // }}}
static int struct_destroy(backend_t *backend){ // {{{
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int struct_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	hash_t                *struct_hash   = NULL;
	struct_userdata       *userdata      = (struct_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT,    struct_hash,          config, HK(structure));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->buffer,     config, HK(buffer));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key_values, config, HK(values));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->size,       config, HK(size));
	hash_data_copy(ret, TYPE_UINTT,    userdata->lazy,       config, HK(lazy));
	
	userdata->structure  = struct_hash;
	userdata->values     = (userdata->key_values == 0) ? STRUCT_VALUES_WHOLE : STRUCT_VALUES_ONE;
	
	if(userdata->buffer == 0 && userdata->lazy == 0)
		return error("backend struct parameter buffer invalid");
	if(userdata->structure == NULL)
		return error("backend struct parameter structure invalid");
	
	return 0;
} // }}}

static ssize_t struct_backend_pack(backend_t *backend, request_t *request){
	ssize_t          ret;
	size_t           struct_size;
	data_t          *buffer;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
		
	switch(userdata->values){
		case STRUCT_VALUES_WHOLE: values = request; break;
		case STRUCT_VALUES_ONE:
			hash_data_copy(ret, TYPE_HASHT, values, request, userdata->key_values);
			if(ret != 0)
				return warning("hash with keys not supplied");
			break;
	};
	
	if(userdata->lazy == 1){
		request_t r_next[] = {
			{ userdata->buffer, DATA_STRUCTT(userdata->structure, values) },
			hash_next(request)
		};
		
		return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
	}else{
		buffer = hash_data_find(request, userdata->buffer);
		if(buffer != NULL){
			if( (struct_size = struct_pack(userdata->structure, values, buffer)) == 0)
				return error("struct_pack failed");
			
			request_t new_request[] = {
				{ userdata->size, DATA_PTR_SIZET(&struct_size) },
				hash_next(request)
			};
			
			return ( (ret = backend_pass(backend, new_request)) < 0) ? ret : -EEXIST;
		}
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	}
}

static ssize_t struct_backend_unpack(backend_t *backend, request_t *request){
	ssize_t          ret, ret2;
	data_t          *buffer;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	ret = (ret = backend_pass(backend, request)) < 0 ? ret : -EEXIST;
	
	buffer = hash_data_find(request, userdata->buffer);
	if(buffer != NULL){
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:
				hash_data_copy(ret2, TYPE_HASHT, values, request, userdata->key_values);
				if(ret2 != 0)
					return warning("hash with keys not supplied");
				break;
		};
		
		if(struct_unpack(userdata->structure, values, buffer) == 0)
			return error("struct_unpack failed");
	}
	
	return ret;
}

backend_t structs_proto = {
	.class          = "data/structs", // this backend would be deprecated
	.supported_api  = API_CRWD,
	.func_init      = &struct_init,
	.func_configure = &struct_configure,
	.func_destroy   = &struct_destroy,
	{
		.func_create = &struct_backend_pack,
		.func_set    = &struct_backend_pack,
		.func_get    = &struct_backend_unpack
	}
};

backend_t struct_pack_proto = {
	.class          = "data/struct_pack",
	.supported_api  = API_HASH,
	.func_init      = &struct_init,
	.func_configure = &struct_configure,
	.func_destroy   = &struct_destroy,
	.backend_type_hash = {
		.func_handler = &struct_backend_pack
	}
};

backend_t struct_unpack_proto = {
	.class          = "data/struct_unpack",
	.supported_api  = API_HASH,
	.func_init      = &struct_init,
	.func_configure = &struct_configure,
	.func_destroy   = &struct_destroy,
	.backend_type_hash = {
		.func_handler = &struct_backend_unpack
	}
};
