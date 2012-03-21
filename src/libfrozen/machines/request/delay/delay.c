#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_delay request/delay
 */
/**
 * @ingroup mod_machine_delay
 * @page page_delay_info Description
 *
 * This machine delay request for given time
 */
/**
 * @ingroup mod_machine_delay
 * @page page_delay_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/delay",
 *              sleep                   = (uint_t)'0',          // delay time in seconds, default 0 seconds
 *              usleep                  = (uint_t)'0'           // delay time in microseconds, default 0 microseconds  (1000000 == 1 sec)
 * }
 * @delaycode
 */

#define ERRORS_MODULE_ID 18
#define ERRORS_MODULE_NAME "request/delay"

typedef struct delay_userdata {
	data_t                 sleep;
	data_t                 usleep;
} delay_userdata;

static ssize_t delay_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              tsleep            = 0;
	uintmax_t              tusleep           = 0;
	delay_userdata        *userdata          = (delay_userdata *)machine->userdata;
	
	request_enter_context(request);
		
		data_convert(ret, TYPE_UINTT, tsleep,  &userdata->sleep);
		data_convert(ret, TYPE_UINTT, tusleep, &userdata->usleep);
		(void)ret;
		
		sleep(tsleep);
		usleep(tusleep);
		
	request_leave_context();
	return machine_pass(machine, request);
} // }}}

static ssize_t delay_init(machine_t *machine){ // {{{
	delay_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(delay_userdata))) == NULL)
		return error("calloc failed");
	
	data_set_void(&userdata->sleep);
	data_set_void(&userdata->usleep);
	return 0;
} // }}}
static ssize_t delay_destroy(machine_t *machine){ // {{{
	delay_userdata      *userdata = (delay_userdata *)machine->userdata;
	
	data_free(&userdata->sleep);
	data_free(&userdata->usleep);
	free(userdata);
	return 0;
} // }}}
static ssize_t delay_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	delay_userdata        *userdata          = (delay_userdata *)machine->userdata;
	
	hash_holder_consume(ret, userdata->sleep,  config, HK(sleep));
	hash_holder_consume(ret, userdata->usleep, config, HK(usleep));
	(void)ret;
	return 0;
} // }}}

machine_t delay_proto = {
	.class          = "request/delay",
	.supported_api  = API_HASH,
	.func_init      = &delay_init,
	.func_destroy   = &delay_destroy,
	.func_configure = &delay_configure,
	.machine_type_hash = {
		.func_handler = &delay_handler
	},
};

