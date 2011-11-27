#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_transfer data/transfer
 */
/**
 * @ingroup mod_backend_transfer
 * @page page_transfer_info Description
 *
 * This is passive module which transfer data from one backend to another on every incoming request.
 */
/**
 * @ingroup mod_backend_transfer
 * @page page_transfer_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/transfer",
 *              source                  = (backend_t)'',      # source backend, default - underlying backend
 *              destination             = (backend_t)''       # destination backend, default - underlying backend
 * }
 * @endcode
 */

#define EMODULE 39

typedef struct transfer_userdata {
	backend_t             *source;
	backend_t             *destination;
} transfer_userdata;

static int transfer_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(transfer_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int transfer_destroy(backend_t *backend){ // {{{
	transfer_userdata        *userdata          = (transfer_userdata *)backend->userdata;
	
	if(userdata->source)
		backend_destroy(userdata->source);
	if(userdata->destination)
		backend_destroy(userdata->destination);
	
	free(userdata);
	return 0;
} // }}}
static int transfer_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	transfer_userdata     *userdata          = (transfer_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_BACKENDT,   userdata->source,        config, HK(source));
	hash_data_copy(ret, TYPE_BACKENDT,   userdata->destination,   config, HK(destination));
	return 0;
} // }}}

static ssize_t transfer_handler(backend_t *backend, request_t *request){ // {{{
	backend_t             *source;
	backend_t             *destination;
	transfer_userdata     *userdata          = (transfer_userdata *)backend->userdata;
	
	source      = userdata->source      ? userdata->source      : backend; // backend have no handlers, so it will pass to underlying
	destination = userdata->destination ? userdata->destination : backend;
	
	data_t  d_source      = DATA_BACKENDT(source);
	data_t  d_destination = DATA_BACKENDT(destination);
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_destination };
	return data_query(&d_source, &r_transfer);
} // }}}
static ssize_t transfer_fast_handler(backend_t *backend, void *hargs){ // {{{
	return transfer_handler(backend, NULL);
} // }}}

backend_t transfer_proto = {
	.class          = "data/transfer",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &transfer_init,
	.func_configure = &transfer_configure,
	.func_destroy   = &transfer_destroy,
	.backend_type_hash = {
		.func_handler = &transfer_handler
	},
	.backend_type_fast = {
		.func_handler = &transfer_fast_handler
	}
};

