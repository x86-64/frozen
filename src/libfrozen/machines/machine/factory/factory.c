#include <libfrozen.h>

#define EMODULE 33

typedef struct factory_userdata {
	config_t              *machine_config;
	hashkey_t              output;
} factory_userdata;

static int factory_init(machine_t *machine){ // {{{
	factory_userdata      *userdata;
	if((userdata = machine->userdata = calloc(1, sizeof(factory_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->output = HK(machine);
	return 0;
} // }}}
static int factory_destroy(machine_t *machine){ // {{{
	factory_userdata      *userdata = (factory_userdata *)machine->userdata;
	
	hash_free(userdata->machine_config);
	free(userdata);
	return 0;
} // }}}
static int factory_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	factory_userdata      *userdata          = (factory_userdata *)machine->userdata;
	
	hash_data_get    (ret, TYPE_HASHKEYT,  userdata->output,         config, HK(output));
	hash_data_consume(ret, TYPE_HASHT,     userdata->machine_config, config, HK(config));
	if(ret != 0)
		return error("HK(config) not supplied");
	
	return 0;
} // }}}

static ssize_t factory_handler(machine_t *machine, request_t *request){ // {{{
	data_t                *output;
	machine_t             *child;
	factory_userdata      *userdata = (factory_userdata *)machine->userdata;
	
	if( (output = hash_data_find(request, userdata->output)) == NULL)
		return -EINVAL;
	
	if( output->type != TYPE_MACHINET )
		return -EINVAL;

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
	
	output->ptr = child;
	return 0;
} // }}}

machine_t factory_proto = {
	.class          = "machine/factory",
	.supported_api  = API_HASH,
	.func_init      = &factory_init,
	.func_destroy   = &factory_destroy,
	.func_configure = &factory_configure,
	.machine_type_hash = {
		.func_handler = &factory_handler
	}
};

