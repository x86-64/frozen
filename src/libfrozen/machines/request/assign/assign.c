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
 * 	        before     = {                            // this key-values override existing key-values with same key, optional
 *                      buffer = (string_t)'hello'
 * 	        },
 * 	        after      = {                            // this key-values would be used if there is no other with such key, optional
 *                      buffer = (string_t)'hello'
 * 	        },
 * 	        request    = {                            // override whole request with this one (before and after not taken in account), optional
 *                      buffer = (string_t)'hello'
 * 	        },
 *              copy       = (uint_t)"1"                  // copy request on each call, default 0
 * 	}
 * @endcode
 */

#define EMODULE         43

typedef struct assign_userdata {
	hash_t                *before;
	hash_t                *after;
	hash_t                *request;
	uintmax_t              copy;
} assign_userdata;

static ssize_t assign_init(machine_t *machine){ // {{{
	if( (machine->userdata = calloc(1, sizeof(assign_userdata))) == NULL)
		return error("calloc returns null");
	
	return 0;
} // }}}
static ssize_t assign_destroy(machine_t *machine){ // {{{
	assign_userdata       *userdata          = (assign_userdata *)machine->userdata;
	
	hash_free(userdata->before);
	hash_free(userdata->after);
	hash_free(userdata->request);
	free(userdata);
	return 0;
} // }}}
static ssize_t assign_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	assign_userdata       *userdata          = (assign_userdata *)machine->userdata;
	
	hash_data_consume(ret, TYPE_HASHT, userdata->before,   config, HK(before));
	hash_data_consume(ret, TYPE_HASHT, userdata->after,    config, HK(after));
	hash_data_consume(ret, TYPE_HASHT, userdata->request,  config, HK(request));
	hash_data_get    (ret, TYPE_UINTT, userdata->copy,     config, HK(copy));
	return 0;
} // }}}

static ssize_t assign_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	assign_userdata       *userdata          = (assign_userdata *)machine->userdata;
	hash_t                *req;
	hash_t                *req_before;
	hash_t                *req_after;
	
	if(userdata->request != NULL){
		if(userdata->copy != 0){
			req = userdata->request;
			
			if( req && (req = hash_copy(req)) == NULL)
				return -ENOMEM;
			
			ret = machine_pass(machine, userdata->request);
			hash_free(req);
			return ret;
		}else
			return machine_pass(machine, userdata->request);
	}else{
		req_before = userdata->before;
		req_after  = userdata->after;
		
		if(userdata->copy != 0){
			if(
				(req_before && (req_before = hash_copy(req_before)) == NULL) ||
				(req_after  && (req_after  = hash_copy(req_after))  == NULL)
			)
				return -ENOMEM;
		}
		
		hash_t r_next[] = {
			hash_inline(req_before),
			hash_inline(request),
			hash_inline(req_after),
			hash_end
		};
		
		ret = machine_pass(machine, r_next);
		
		if(userdata->copy != 0){
			hash_free(req_before);
			hash_free(req_after);
		}
		return ret;
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

