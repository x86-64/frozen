#include <libfrozen.h>

/* this module must be used only as example */

static int null_init(machine_t *machine){
	(void)machine;
	return 0;
}

static int null_destroy(machine_t *machine){
	(void)machine;
	return 0;
}

static int null_configure(machine_t *machine, config_t *config){
	(void)machine; (void)config;
	return 0;
}

static ssize_t null_create(machine_t *machine, request_t *request){
	ssize_t       ret;
	size_t        value;
	
	hash_data_copy(ret, TYPE_SIZET, value, request, HK(size));
	if(ret == 0 && value == 0x0000BEEF)
		return value;
	
	return ( (ret = machine_pass(machine, request)) < 0) ? ret : -EEXIST;
}

static ssize_t null_read(machine_t *machine, request_t *request){
	return 0;
}

machine_t null_proto = {
	.class          = "examples/null",
	.supported_api  = API_CRWD,
	.func_init      = &null_init,
	.func_configure = &null_configure,
	.func_destroy   = &null_destroy,
	{
		.func_create = &null_create,
		.func_get    = &null_read
	}
};

