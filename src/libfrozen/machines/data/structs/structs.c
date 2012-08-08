#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_structs data/structs
 */
/**
 * @ingroup mod_machine_structs
 * @page page_structs_info Description
 *
 * This machine pack and unpack user requests accoring defined structure. It pack values as it is, to convert use @ref mod_machine_convert.
 */
/**
 * @ingroup mod_machine_structs
 * @page page_structs_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   =
 *                                        "data/struct_pack",
 *                                        "data/struct_unpack",
 *              buffer                  = (hashkey_t)'buffer', # input/output request key name, default "buffer"
 *              values                  = (hashkey_t)'some',   # if supplied - request key name to take/write values from/to, default take from request
 *              size                    = (hashkey_t)'size',   # if supplied - add size of packed structure to request with key
 *              lazy                    = (uint_t)'1',         # use lazy packing (no size would be avaliable), default 0
 *              structure               = {                     # structure to pack/unpack to/from
 *                     keyname = {                              # - first field is keyname with default value of 10 and FORMAT(native)
 *                                  default = (uint_t)'10',
 *                                  format  = (format_t)'native'
 *                               }
 *                     key2    = { },                           # - second field is anything without default value and FORMAT(packed)
 *                     ...
 *              }
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 5
#define ERRORS_MODULE_NAME "data/structs"

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

static ssize_t struct_init(machine_t *machine){ // {{{
	struct_userdata       *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(struct_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->buffer = HK(buffer);
	
	return 0;
} // }}}
static ssize_t struct_destroy(machine_t *machine){ // {{{
	struct_userdata *userdata = (struct_userdata *)machine->userdata;
	
	hash_free(userdata->structure);
	free(userdata);
	return 0;
} // }}}
static ssize_t struct_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	struct_userdata       *userdata      = (struct_userdata *)machine->userdata;
	
	hash_data_consume(ret, TYPE_HASHT,    userdata->structure,  config, HK(structure));
	hash_data_get    (ret, TYPE_HASHKEYT, userdata->buffer,     config, HK(buffer));
	hash_data_get    (ret, TYPE_HASHKEYT, userdata->key_values, config, HK(values));
	hash_data_get    (ret, TYPE_HASHKEYT, userdata->size,       config, HK(size));
	hash_data_get    (ret, TYPE_UINTT,    userdata->lazy,       config, HK(lazy));
	
	userdata->values     = (userdata->key_values == 0) ? STRUCT_VALUES_WHOLE : STRUCT_VALUES_ONE;
	
	if(userdata->buffer == 0 && userdata->lazy == 0)
		return error("machine struct parameter buffer invalid");
	if(userdata->structure == NULL)
		return error("machine struct parameter structure invalid");
	
	return 0;
} // }}}

static ssize_t struct_machine_pack(machine_t *machine, request_t *request){
	ssize_t          ret;
	size_t           struct_size;
	data_t          *buffer;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)machine->userdata;
		
	switch(userdata->values){
		case STRUCT_VALUES_WHOLE: values = request; break;
		case STRUCT_VALUES_ONE:
			hash_data_get(ret, TYPE_HASHT, values, request, userdata->key_values);
			if(ret != 0)
				return error("hash with keys not supplied");
			break;
	};
	
	if(userdata->lazy == 1){
		request_t r_next[] = {
			{ userdata->buffer, DATA_STRUCTT(userdata->structure, values) },
			hash_next(request)
		};
		
		return machine_pass(machine, r_next);
	}else{
		buffer = hash_data_find(request, userdata->buffer);
		if(buffer != NULL){
			if( (struct_size = struct_pack(userdata->structure, values, buffer)) == 0)
				return error("struct_pack failed");
			
			request_t new_request[] = {
				{ userdata->size, DATA_SIZET(struct_size) },
				hash_next(request)
			};
			
			return machine_pass(machine, new_request);
		}
		return machine_pass(machine, request);
	}
}

static ssize_t struct_machine_unpack(machine_t *machine, request_t *request){
	ssize_t          ret;
	data_t          *buffer;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)machine->userdata;
	
	buffer = hash_data_find(request, userdata->buffer);
	if(buffer != NULL){
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:
				hash_data_get(ret, TYPE_HASHT, values, request, userdata->key_values);
				if(ret != 0)
					return error("hash with keys not supplied");
				break;
		};
		
		if(struct_unpack(userdata->structure, values, buffer) == 0)
			return error("struct_unpack failed");
	}
	
	return machine_pass(machine, request);
}

machine_t struct_pack_proto = {
	.class          = "data/struct_pack",
	.supported_api  = API_HASH,
	.func_init      = &struct_init,
	.func_configure = &struct_configure,
	.func_destroy   = &struct_destroy,
	.machine_type_hash = {
		.func_handler = &struct_machine_pack
	}
};

machine_t struct_unpack_proto = {
	.class          = "data/struct_unpack",
	.supported_api  = API_HASH,
	.func_init      = &struct_init,
	.func_configure = &struct_configure,
	.func_destroy   = &struct_destroy,
	.machine_type_hash = {
		.func_handler = &struct_machine_unpack
	}
};
