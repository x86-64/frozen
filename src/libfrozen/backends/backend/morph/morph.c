#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_morph backend/morph
 */
/**
 * @ingroup mod_backend_morph
 * @page page_morph_info Description
 *
 * This backend wait for first request, combine it with configuration supplied by user and transform itself to resulting backend.
 * All further requests processed as there was no morph at all. First request override same parameters from user supplied configuration.
 *
 */
/**
 * @ingroup mod_backend_morph
 * @page page_morph_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "backend/morph",
 *              config                  = { ... },            # backend configuration
 *              pass_first              = (uint_t)'0',        # pass also first request to new backend, default 1
 * }
 * @endcode
 */

#define EMODULE 20

typedef struct morph_userdata {
	uintmax_t              running;
	uintmax_t              pass_first;
	config_t              *backend_config;
} morph_userdata;

static int morph_init(backend_t *backend){ // {{{
	morph_userdata      *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(morph_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->pass_first = 1;
	return 0;
} // }}}
static int morph_destroy(backend_t *backend){ // {{{
	morph_userdata      *userdata = (morph_userdata *)backend->userdata;
	free(userdata);
	return 0;
} // }}}
static int morph_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	morph_userdata      *userdata          = (morph_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_UINTT, userdata->pass_first,     config, HK(pass_first));
	hash_data_copy(ret, TYPE_HASHT, userdata->backend_config, config, HK(config));
	if(ret != 0)
		return error("HK(config) not supplied");
	
	//userdata->running = 0; // calloc in init already set it
	return 0;
} // }}}

static ssize_t morph_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	backend_t             *child;
	morph_userdata      *userdata = (morph_userdata *)backend->userdata;

	if(userdata->running == 0){
		config_t  child_config[] = {
			{ 0, DATA_HASHT(
				hash_inline(request),
				hash_inline(userdata->backend_config),
				hash_end
			)},
			hash_end
		};
		child = backend_new(child_config);
		
		if(child == NULL)
			return error("child creation error");

		backend_connect(backend, child); // on destroy - no need to disconnect or call _destory, core will automatically do it
		userdata->running = 1;

		if(userdata->pass_first == 0)
			return 0;
	}

	return ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
} // }}}

backend_t morph_proto = {
	.class          = "backend/morph",
	.supported_api  = API_HASH,
	.func_init      = &morph_init,
	.func_destroy   = &morph_destroy,
	.func_configure = &morph_configure,
	.backend_type_hash = {
		.func_handler = &morph_handler
	}
};

