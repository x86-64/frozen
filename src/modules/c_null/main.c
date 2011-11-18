#include <libfrozen.h>

#define EMODULE 100       // emodule for errors, pick up non-used number

typedef struct null_userdata {       // backend userdata structure
	uintmax_t              testparam;
} null_userdata;

static int null_init(backend_t *backend){ // {{{
	null_userdata         *userdata;

	if((userdata = backend->userdata = calloc(1, sizeof(null_userdata))) == NULL)
		return error("calloc failed");
	
	// default values
	userdata->testparam = 10;
	return 0;
} // }}}
static int null_destroy(backend_t *backend){ // {{{
	null_userdata         *userdata          = (null_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int null_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	null_userdata         *userdata          = (null_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_UINTT, userdata->testparam, config, HK(test));
	return 0;
} // }}}

static ssize_t null_handler(backend_t *backend, request_t *request){ // {{{
	null_userdata         *userdata = (null_userdata *)backend->userdata;
	
	return (ssize_t)userdata->testparam;
} // }}}

static backend_t null_proto = {                 // NOTE need static or unique name
	.class          = "modules/c_null",
	.supported_api  = API_HASH,
	.func_init      = &null_init,
	.func_configure = &null_configure,
	.func_destroy   = &null_destroy,
	.backend_type_hash = {
		.func_handler = &null_handler
	}
};

int main(void){
	class_register(&null_proto);
	return 0;
}
