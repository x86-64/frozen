#include <libfrozen.h>

#define EMODULE 100       // emodule for errors, pick up non-used number

typedef struct null_userdata {       // machine userdata structure
	uintmax_t              testparam;
} null_userdata;

static int null_init(machine_t *machine){ // {{{
	null_userdata         *userdata;

	if((userdata = machine->userdata = calloc(1, sizeof(null_userdata))) == NULL)
		return error("calloc failed");
	
	// default values
	userdata->testparam = 10;
	return 0;
} // }}}
static int null_destroy(machine_t *machine){ // {{{
	null_userdata         *userdata          = (null_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int null_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	null_userdata         *userdata          = (null_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_UINTT, userdata->testparam, config, HK(test));
	return 0;
} // }}}

static ssize_t null_handler(machine_t *machine, request_t *request){ // {{{
	null_userdata         *userdata = (null_userdata *)machine->userdata;
	
	return (ssize_t)userdata->testparam;
} // }}}

static machine_t c_null_proto = {
	.class          = "modules/c_null",
	.supported_api  = API_HASH,
	.func_init      = &null_init,
	.func_configure = &null_configure,
	.func_destroy   = &null_destroy,
	.machine_type_hash = {
		.func_handler = &null_handler
	}
};

int main(void){
	class_register(&c_null_proto);
	return 0;
}
