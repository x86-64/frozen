#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_hashtable index/hashtable
 */
/**
 * @ingroup mod_machine_hashtable
 * @page page_hashtable_info Description
 *
 * This is simpliest index avaliable. For given value (i.e. from data hasher machine) it calculate
 * offset in fixed size table and pass request. No shoping, nor collision detection. It more 
 * alike bloom filter, but for bigger than bit values.
 *
 */
/**
 * @ingroup mod_machine_hashtable
 * @page page_hashtable_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "index/hashtable",
 *              input                   = (hashkey_t)'keyid', # key for input, default "key"
 *              nelements               = (uint_t)'100',      # size of hash table
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID              31
#define ERRORS_MODULE_NAME "index/hashtable"

typedef struct hashtable_userdata {
	hashkey_t              input;
	uintmax_t              hashtable_size;
} hashtable_userdata;

static ssize_t hashtable_init(machine_t *machine){ // {{{
	hashtable_userdata    *userdata = machine->userdata = calloc(1, sizeof(hashtable_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->input = HK(key);
	return 0;
} // }}}
static ssize_t hashtable_destroy(machine_t *machine){ // {{{
	hashtable_userdata    *userdata = (hashtable_userdata *)machine->userdata;

	free(userdata);
	return 0;
} // }}}
static ssize_t hashtable_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	hashtable_userdata    *userdata        = (hashtable_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->input,          config, HK(input));
       	hash_data_get(ret, TYPE_UINTT,    userdata->hashtable_size, config, HK(nelements));
	
	if(userdata->hashtable_size == 0)
		return error("invalid hashtable size");
	return 0;
} // }}}

static ssize_t hashtable_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              d_input;
	hashtable_userdata    *userdata          = (hashtable_userdata *)machine->userdata;

	hash_data_get(ret, TYPE_UINTT,   d_input, request, userdata->input);
	if(ret == 0){
		d_input = d_input % userdata->hashtable_size;
		
		request_t r_next[] = {
			{ userdata->input,  DATA_PTR_UINTT(&d_input) },
			hash_next(request)
		};
		return machine_pass(machine, r_next);
	}
	return machine_pass(machine, request);
} // }}}

machine_t hashtable_proto = {
	.class          = "index/hashtable",
	.supported_api  = API_HASH,
	.func_init      = &hashtable_init,
	.func_configure = &hashtable_configure,
	.func_destroy   = &hashtable_destroy,
	.machine_type_hash = {
		.func_handler = &hashtable_handler
	}
};

