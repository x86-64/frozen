#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_iterate request/iterate
 *
 * This backend pass requests to childs iterativly until successful error code got.
 */
/**
 * @ingroup mod_backend_iterate
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

static int iterate_init(backend_t *backend){ // {{{
	iterate_userdata      *userdata;
	
	if( (userdata = backend->userdata = calloc(1, sizeof(iterate_userdata))) == NULL)
		return error("calloc returns null");
	
	userdata->error_code = 0;
	return 0;
} // }}}
static int iterate_destroy(backend_t *backend){ // {{{
	iterate_userdata      *userdata          = (iterate_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int iterate_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	iterate_userdata      *userdata          = (iterate_userdata *)backend->userdata;

	hash_data_copy(ret, TYPE_SIZET, userdata->error_code, config, HK(error));
	return 0;
} // }}}

static ssize_t iterate_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              i, lsz;
	backend_t            **lchilds;
	iterate_userdata      *userdata          = (iterate_userdata *)backend->userdata;
	
	list_rdlock(&backend->childs);
		
		if( (lsz = list_count(&backend->childs)) != 0){
			lchilds = alloca( sizeof(backend_t *) * lsz );
			list_flatten(&backend->childs, (void **)lchilds, lsz);
		}
		
	list_unlock(&backend->childs);
	
	if(lsz == 0)
		return error("no childs");
	
	for(i = 0; i < lsz; i++){
		if( (ret = backend_query(lchilds[i], request)) == userdata->error_code)
			return ret;
	}
	return -EEXIST;
} // }}}

backend_t iterate_proto = {
	.class          = "request/iterate",
	.supported_api  = API_HASH,
	.func_init      = &iterate_init,
	.func_configure = &iterate_configure,
	.func_destroy   = &iterate_destroy,
	.backend_type_hash = {
		.func_handler  = &iterate_handler
	}
};

