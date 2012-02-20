#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_morph machine/morph
 */
/**
 * @ingroup mod_machine_morph
 * @page page_morph_info Description
 *
 * This machine wait for first request, combine it with configuration supplied by user and transform itself to resulting machine.
 * All further requests processed as there was no morph at all. First request override same parameters from user supplied configuration.
 *
 */
/**
 * @ingroup mod_machine_morph
 * @page page_morph_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "machine/morph",
 *              config                  = { ... },            # machine configuration
 *              pass_first              = (uint_t)'0',        # pass also first request to new machine, default 1
 * }
 * @endcode
 */

#define EMODULE 20

typedef struct morph_userdata {
	uintmax_t              running;
	uintmax_t              pass_first;
	config_t              *machine_config;
} morph_userdata;

static ssize_t morph_init(machine_t *machine){ // {{{
	morph_userdata      *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(morph_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->pass_first = 1;
	return 0;
} // }}}
static ssize_t morph_destroy(machine_t *machine){ // {{{
	morph_userdata      *userdata = (morph_userdata *)machine->userdata;
	
	hash_free(userdata->machine_config);
	free(userdata);
	return 0;
} // }}}
static ssize_t morph_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	morph_userdata      *userdata          = (morph_userdata *)machine->userdata;
	
	hash_data_get    (ret, TYPE_UINTT, userdata->pass_first,     config, HK(pass_first));
	hash_data_consume(ret, TYPE_HASHT, userdata->machine_config, config, HK(config));
	if(ret != 0)
		return error("HK(config) not supplied");
	
	//userdata->running = 0; // calloc in init already set it
	return 0;
} // }}}

static ssize_t morph_handler(machine_t *machine, request_t *request){ // {{{
	machine_t             *child;
	morph_userdata        *userdata = (morph_userdata *)machine->userdata;

	if(userdata->running == 0){
		config_t  child_config[] = {
			{ 0, DATA_HASHT(
				hash_inline(request),
				hash_inline(userdata->machine_config),
				hash_end
			)},
			hash_end
		};
		child = shop_new(child_config);
		
		if(child == NULL)
			return error("child creation error");

		//machine_connect(machine, child); // on destroy - no need to disconnect or call _destory, core will automatically do it
		userdata->running = 1;

		if(userdata->pass_first == 0)
			return 0;
	}

	return machine_pass(machine, request);
} // }}}

machine_t morph_proto = {
	.class          = "machine/morph",
	.supported_api  = API_HASH,
	.func_init      = &morph_init,
	.func_destroy   = &morph_destroy,
	.func_configure = &morph_configure,
	.machine_type_hash = {
		.func_handler = &morph_handler
	}
};

