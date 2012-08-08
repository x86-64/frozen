#include <libfrozen.h>
#include <murmur2.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_murmur hash/murmur
 */
/**
 * @ingroup mod_machine_murmur
 * @page page_murmur_info Description
 *
 * This is implementation of murmur2 hash. It hash input data and add resulting hash to request.
 */
/**
 * @ingroup mod_machine_murmur
 * @page page_murmur_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = 
 *                                        "hash/murmur2_32",
 *                                        "hash/murmur2_64",
 *              input                   = (hashkey_t)'buffer', # input key name
 *              output                  = (hashkey_t)'keyid',  # output key name
 *              fatal                   = (uint_t)'1',         # interrupt request if input not present, default 0
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 22
#define ERRORS_MODULE_NAME "hash/murmur"

typedef struct murmur_userdata {
	hashkey_t             input;
	hashkey_t             output;
	uintmax_t              fatal;
} murmur_userdata;

static ssize_t murmur_init(machine_t *machine){ // {{{
	if((machine->userdata = calloc(1, sizeof(murmur_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static ssize_t murmur_destroy(machine_t *machine){ // {{{
	murmur_userdata       *userdata = (murmur_userdata *)machine->userdata;

	free(userdata);
	return 0;
} // }}}
static ssize_t murmur_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	murmur_userdata       *userdata          = (murmur_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->input,    config, HK(input));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->output,   config, HK(output));
	hash_data_get(ret, TYPE_UINTT,    userdata->fatal,    config, HK(fatal));
	
	userdata->fatal   = ( userdata->fatal == 0 ) ? 0 : 1;
	return 0;
} // }}}

static ssize_t murmur2_32_handler(machine_t *machine, request_t *request){ // {{{
	uint32_t               hash              = 0;
	data_t                *key               = NULL;
	murmur_userdata       *userdata          = (murmur_userdata *)machine->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return machine_pass(machine, request);
		return error("input key not supplied");
	}
	
	hash = MurmurHash2(key, 0);
	
	request_t r_next[] = {
		{ userdata->output, DATA_UINT32T(hash) },
		hash_next(request)
	};
	return machine_pass(machine, r_next);
} // }}}
static ssize_t murmur2_64_handler(machine_t *machine, request_t *request){ // {{{
	uint64_t               hash              = 0;
	data_t                *key               = NULL;
	murmur_userdata       *userdata          = (murmur_userdata *)machine->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return machine_pass(machine, request);
		return error("input key not supplied");
	}
	
	hash = MurmurHash64A(key, 0);
	
	request_t r_next[] = {
		{ userdata->output, DATA_UINT64T(hash) },
		hash_next(request)
	};
	return machine_pass(machine, r_next);
} // }}}

machine_t murmur2_32_proto = {
	.class          = "hash/murmur2_32",
	.supported_api  = API_HASH,
	.func_init      = &murmur_init,
	.func_destroy   = &murmur_destroy,
	.func_configure = &murmur_configure,
	.machine_type_hash = {
		.func_handler  = &murmur2_32_handler
	}
};

machine_t murmur2_64_proto = {
	.class          = "hash/murmur2_64",
	.supported_api  = API_HASH,
	.func_init      = &murmur_init,
	.func_destroy   = &murmur_destroy,
	.func_configure = &murmur_configure,
	.machine_type_hash = {
		.func_handler  = &murmur2_64_handler
	}
};

