#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_assign request/assign
 *
 * Assign module set new request's keys.
 */
/**
 * @ingroup mod_machine_assign
 * @page page_assign_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class      = "request/assign",
 * 	        before     = {                            # this key-values override existing key-values with same key, optional
 *                      buffer = (string_t)'hello'
 * 	        },
 * 	        after      = {                            # this key-values would be used if there is no other with such key, optional
 *                      buffer = (string_t)'hello'
 * 	        },
 * 	        request    = {                            # override whole request with this one (before and after not taken in account), optional
 *                      buffer = (string_t)'hello'
 * 	        }
 * 	}
 * @endcode
 */

#define EMODULE         43

typedef struct assign_userdata {
	hash_t                *before;
	hash_t                *after;
	hash_t                *request;
} assign_userdata;

static int assign_init(machine_t *machine){ // {{{
	if( (machine->userdata = calloc(1, sizeof(assign_userdata))) == NULL)
		return error("calloc returns null");
	
	return 0;
} // }}}
static int assign_destroy(machine_t *machine){ // {{{
	assign_userdata       *userdata          = (assign_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int assign_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	assign_userdata       *userdata          = (assign_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_HASHT, userdata->before,   config, HK(before));
	hash_data_copy(ret, TYPE_HASHT, userdata->after,    config, HK(after));
	hash_data_copy(ret, TYPE_HASHT, userdata->request,  config, HK(request));
	return 0;
} // }}}

static ssize_t assign_handler(machine_t *machine, request_t *request){ // {{{
	assign_userdata       *userdata          = (assign_userdata *)machine->userdata;
	
	if(userdata->request != NULL){
		return machine_pass(machine, userdata->request);
	}else{
		hash_t r_next[] = {
			hash_inline(userdata->before),
			hash_inline(request),
			hash_inline(userdata->after),
			hash_end
		};
		
		return machine_pass(machine, r_next);
	}
} // }}}

machine_t assign_proto = {
	.class          = "request/assign",
	.supported_api  = API_HASH,
	.func_init      = &assign_init,
	.func_configure = &assign_configure,
	.func_destroy   = &assign_destroy,
	.machine_type_hash = {
		.func_handler = &assign_handler
	}
};

