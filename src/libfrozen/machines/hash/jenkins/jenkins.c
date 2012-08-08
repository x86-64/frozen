#include <libfrozen.h>
#include <hash_jenkins.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_jenkins hash/jenkins
 */
/**
 * @ingroup mod_machine_jenkins
 * @page page_jenkins_info Description
 *
 * This is implementation of jenkins hash. It hash input data and add resulting hash to request.
 */
/**
 * @ingroup mod_machine_jenkins
 * @page page_jenkins_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = 
 *                                        "hash/jenkins_32",
 *                                        "hash/jenkins_64",
 *              input                   = (hashkey_t)'buffer', # input key name
 *              output                  = (hashkey_t)'keyid',  # output key name
 *              fatal                   = (uint_t)'1',         # interrupt request if input not present, default 0
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 23
#define ERRORS_MODULE_NAME "hash/jenkins"

typedef struct jenkins_userdata {
	hashkey_t             input;
	hashkey_t             output;
	uintmax_t              fatal;
} jenkins_userdata;

static ssize_t jenkins_init(machine_t *machine){ // {{{
	if((machine->userdata = calloc(1, sizeof(jenkins_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static ssize_t jenkins_destroy(machine_t *machine){ // {{{
	jenkins_userdata       *userdata = (jenkins_userdata *)machine->userdata;

	free(userdata);
	return 0;
} // }}}
static ssize_t jenkins_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	jenkins_userdata      *userdata          = (jenkins_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->input,      config, HK(input));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->output,     config, HK(output));
	hash_data_get(ret, TYPE_UINTT,    userdata->fatal,      config, HK(fatal));
	
	userdata->fatal   = ( userdata->fatal == 0 ) ? 0 : 1;
	return 0;
} // }}}

static ssize_t jenkins_32_handler(machine_t *machine, request_t *request){ // {{{
	uint32_t               hash;
	data_t                *key               = NULL;
	jenkins_userdata      *userdata          = (jenkins_userdata *)machine->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return machine_pass(machine, request);
		return error("input key not supplied");
	}
	
	hash = jenkins32_hash(0, key);
	
	request_t r_next[] = {
		{ userdata->output, DATA_UINT32T(hash) },
		hash_next(request)
	};
	return machine_pass(machine, r_next);
} // }}}

static ssize_t jenkins_64_handler(machine_t *machine, request_t *request){ // {{{
	uint64_t               hash;
	data_t                *key               = NULL;
	jenkins_userdata      *userdata          = (jenkins_userdata *)machine->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return machine_pass(machine, request);
		return error("input key not supplied");
	}

	hash = jenkins64_hash(0, key);
	
	request_t r_next[] = {
		{ userdata->output, DATA_UINT64T(hash) },
		hash_next(request)
	};
	return machine_pass(machine, r_next);
} // }}}

machine_t jenkins32_proto = {
	.class          = "hash/jenkins32",
	.supported_api  = API_HASH,
	.func_init      = &jenkins_init,
	.func_destroy   = &jenkins_destroy,
	.func_configure = &jenkins_configure,
	.machine_type_hash = {
		.func_handler  = &jenkins_32_handler
	}
};

machine_t jenkins64_proto = {
	.class          = "hash/jenkins64",
	.supported_api  = API_HASH,
	.func_init      = &jenkins_init,
	.func_destroy   = &jenkins_destroy,
	.func_configure = &jenkins_configure,
	.machine_type_hash = {
		.func_handler  = &jenkins_64_handler
	}
};

