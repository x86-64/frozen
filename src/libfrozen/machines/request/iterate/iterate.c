#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_iterate request/iterate
 *
 * This machine pass requests to childs iterativly until successful error code got.
 */
/**
 * @ingroup mod_machine_iterate
 * @page page_iterate_config Balancer configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class      = "request/iterate",
 * 	        error      = (int_t)'0',                # wait for this error code
 * 	}
 * @endcode
 */

#define EMODULE         45

typedef struct iterate_userdata {
	ssize_t                error_code;
} iterate_userdata;

static int iterate_init(machine_t *machine){ // {{{
	iterate_userdata      *userdata;
	
	if( (userdata = machine->userdata = calloc(1, sizeof(iterate_userdata))) == NULL)
		return error("calloc returns null");
	
	userdata->error_code = 0;
	return 0;
} // }}}
static int iterate_destroy(machine_t *machine){ // {{{
	iterate_userdata      *userdata          = (iterate_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int iterate_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	iterate_userdata      *userdata          = (iterate_userdata *)machine->userdata;

	hash_data_copy(ret, TYPE_SIZET, userdata->error_code, config, HK(error));
	return 0;
} // }}}

static ssize_t iterate_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              i, lsz;
	machine_t            **lchilds;
	iterate_userdata      *userdata          = (iterate_userdata *)machine->userdata;
	
	list_rdlock(&machine->childs);
		
		if( (lsz = list_count(&machine->childs)) != 0){
			lchilds = alloca( sizeof(machine_t *) * lsz );
			list_flatten(&machine->childs, (void **)lchilds, lsz);
		}
		
	list_unlock(&machine->childs);
	
	if(lsz == 0)
		return error("no childs");
	
	for(i = 0; i < lsz; i++){
		if( (ret = machine_query(lchilds[i], request)) == userdata->error_code)
			return ret;
	}
	return -EEXIST;
} // }}}

machine_t iterate_proto = {
	.class          = "request/iterate",
	.supported_api  = API_HASH,
	.func_init      = &iterate_init,
	.func_configure = &iterate_configure,
	.func_destroy   = &iterate_destroy,
	.machine_type_hash = {
		.func_handler  = &iterate_handler
	}
};

