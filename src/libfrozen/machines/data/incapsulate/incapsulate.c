#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_incapsulate data/incapsulate
 */
/**
 * @ingroup mod_machine_incapsulate
 * @page page_incapsulate_info Description
 *
 * This machine represent fixed size records as one unit items for upper machines
 */
/**
 * @ingroup mod_machine_incapsulate
 * @page page_incapsulate_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/incapsulate",
 *              multiply                = (uint_t)'10',       # item size
 *              key                     = (hashkey_t)'name',  # key to change value (multiply), default offset
 *              key_out                 = (hashkey_t)'name',  # key to change value (multiply), default offset_out
 *              key_to                  = (hashkey_t)'name',  # key to change value (multiply), default offset_to
 *              key_from                = (hashkey_t)'name',  # key to change value (multiply), default offset_from
 *              count                   = (hashkey_t)'name',  # key to change value (divide), default buffer
 *              size                    = (hashkey_t)'name',  # key to change value (divide), default size
 * }
 * @endcode
 */

#define EMODULE 6

typedef struct incap_userdata {
	hashkey_t     key;
	hashkey_t     key_out;
	hashkey_t     key_to;
	hashkey_t     key_from;
	hashkey_t     count;
	hashkey_t     size;
	off_t          multiply;
	data_t         multiply_data;
} incap_userdata;

static int incap_init(machine_t *machine){ // {{{
	incap_userdata        *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(incap_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->key      = HK(offset);
	userdata->key_out  = HK(offset_out);
	userdata->key_to   = HK(offset_to);
	userdata->key_from = HK(offset_from);
	userdata->count    = HK(buffer);
	userdata->size     = HK(size);
	userdata->multiply = 1;
	
	data_t multiply_data = DATA_PTR_OFFT(&userdata->multiply);
	memcpy(&userdata->multiply_data, &multiply_data, sizeof(multiply_data));
	
	return 0;
} // }}}
static int incap_destroy(machine_t *machine){ // {{{
	incap_userdata *userdata = (incap_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int incap_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	incap_userdata        *userdata      = (incap_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key,       config, HK(key));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key_out,   config, HK(key_out));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key_to,    config, HK(key_to));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key_from,  config, HK(key_from));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->count,     config, HK(count));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->size,      config, HK(size));
	hash_data_get(ret, TYPE_OFFT,     userdata->multiply,  config, HK(multiply));
	
	if(userdata->multiply == 0)
		return error("machine incapsulate parameter multiply invalid");
	
	return 0;
} // }}}

static ssize_t incap_handler(machine_t *machine, request_t *request){
	ssize_t                ret;
	uintmax_t              size, new_size;
	data_t                *data;
	incap_userdata        *userdata = (incap_userdata *)machine->userdata;

	if( (data = hash_data_find(request, userdata->size)) != NULL){
		data_get(ret, TYPE_UINTT, size, data);
		
		// round size
		new_size =     size / userdata->multiply;
		new_size = new_size * userdata->multiply;
		if( (size % userdata->multiply) != 0)
			new_size += userdata->multiply;
		
		fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &new_size, sizeof(new_size) };
		data_query(data, &r_write);
	}
	
	fastcall_mul r_mul = { { 3, ACTION_MULTIPLY }, &userdata->multiply_data };
	
	if( (data = hash_data_find(request, userdata->key)) != NULL)
		data_query(data, &r_mul);
	if( (data = hash_data_find(request, userdata->key_to)) != NULL)
		data_query(data, &r_mul);
	if( (data = hash_data_find(request, userdata->key_from)) != NULL)
		data_query(data, &r_mul);
	
	ret = machine_pass(machine, request);
	
	fastcall_div r_div = { { 3, ACTION_DIVIDE }, &userdata->multiply_data };
	
	if( (data = hash_data_find(request, userdata->key_out)) != NULL)
		data_query(data, &r_div);
	if( (data = hash_data_find(request, userdata->count)) != NULL)
		data_query(data, &r_div);
	
	return ret;
}

machine_t incapsulate_proto = {
	.class          = "data/incapsulate",
	.supported_api  = API_HASH,
	.func_init      = &incap_init,
	.func_configure = &incap_configure,
	.func_destroy   = &incap_destroy,
	.machine_type_hash = {
		.func_handler = &incap_handler,
	}
};

