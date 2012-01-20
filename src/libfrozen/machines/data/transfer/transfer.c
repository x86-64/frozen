#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_transfer data/transfer
 */
/**
 * @ingroup mod_machine_transfer
 * @page page_transfer_info Description
 *
 * This is passive module which transfer data from one machine to another on every incoming request.
 */
/**
 * @ingroup mod_machine_transfer
 * @page page_transfer_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/transfer",
 *              source                  = (machine_t)'',      # source machine, default - underlying machine
 *              destination             = (machine_t)''       # destination machine, default - underlying machine
 * }
 * @endcode
 */

#define EMODULE 39

typedef struct transfer_userdata {
	machine_t             *source;
	machine_t             *destination;
} transfer_userdata;

static int transfer_init(machine_t *machine){ // {{{
	if((machine->userdata = calloc(1, sizeof(transfer_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int transfer_destroy(machine_t *machine){ // {{{
	transfer_userdata        *userdata          = (transfer_userdata *)machine->userdata;
	
	if(userdata->source)
		shop_destroy(userdata->source);
	if(userdata->destination)
		shop_destroy(userdata->destination);
	
	free(userdata);
	return 0;
} // }}}
static int transfer_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	transfer_userdata     *userdata          = (transfer_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_MACHINET,   userdata->source,        config, HK(source));
	hash_data_get(ret, TYPE_MACHINET,   userdata->destination,   config, HK(destination));
	
	if(userdata->source == NULL || userdata->destination == NULL)
		return error("source or destination machine invalid");
	
	return 0;
} // }}}

static ssize_t transfer_handler(machine_t *machine, request_t *request){ // {{{
	transfer_userdata     *userdata          = (transfer_userdata *)machine->userdata;
	
	data_t  d_source      = DATA_MACHINET(userdata->source);
	data_t  d_destination = DATA_MACHINET(userdata->destination);
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_destination };
	return data_query(&d_source, &r_transfer);
} // }}}

machine_t transfer_proto = {
	.class          = "data/transfer",
	.supported_api  = API_HASH,
	.func_init      = &transfer_init,
	.func_configure = &transfer_configure,
	.func_destroy   = &transfer_destroy,
	.machine_type_hash = {
		.func_handler = &transfer_handler
	}
};

