#include <libfrozen.h>
#include <shop/pass/pass.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_try shop/try
 */
/**
 * @ingroup mod_machine_try
 * @page page_try_info Description
 *
 * This machine try pass current request to user-supplied shop. Request will reach next machine in two cases:
 * @li User-supplied shop successfully process request and ends up with shop/return
 * @li If error occured - shop/try pass current request with HK(ret) key with error number.
 */
/**
 * @ingroup mod_machine_try
 * @page page_try_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "shop/try",
 *              shop                    =                         // try to this shop
 *                                        (env_t)'machine',       //  - to shop supplied in user request under "machine" hashkey 
 *                                        (machine_t)'name',      //  - to shop named "name"
 *                                        ...
 *              request                 = (hashkey_t)"request",   // request to try, default current request
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 54
#define ERRORS_MODULE_NAME "shop/try"

typedef struct try_userdata {
	data_t                 machine;
	hashkey_t              request;
	hashkey_t              request_out;
	machine_t             *try_end;
} try_userdata;

typedef struct tryend_userdata {
	try_userdata          *userdata;
	machine_t             *machine;
	request_t             *request;
	ssize_t                ret;
} tryend_userdata;

static ssize_t try_end_handler(machine_t *machine, request_t *request){ // {{{
	tryend_userdata       *userdata          = (tryend_userdata *)machine->userdata;
	
	if(userdata->userdata->request == 0){
		userdata->ret = machine_pass(userdata->machine, request);
	}else{
		request_t r_next[] = {
			{ userdata->userdata->request_out, DATA_PTR_HASHT(request) },
			hash_inline(userdata->request),
			hash_end
		};
		userdata->ret = machine_pass(userdata->machine, r_next);
	}
	return 0;
} // }}}

machine_t try_end_proto = {
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &try_end_handler
	}
};

static ssize_t try_init(machine_t *machine){ // {{{
	try_userdata         *userdata;
	tryend_userdata      *te_userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(try_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->request     = 0; 
	userdata->request_out = 0; 
	
	if((userdata->try_end = malloc(sizeof(machine_t))) == NULL){
		free(userdata);
		return error("malloc failed");
	}
	
	if((te_userdata = malloc(sizeof(tryend_userdata))) == NULL){
		free(userdata);
		free(userdata->try_end);
		return error("malloc failed");
	}
	te_userdata->userdata = userdata;
	
	memcpy(userdata->try_end, &try_end_proto, sizeof(machine_t));
	userdata->try_end->userdata = te_userdata;
	return 0;
} // }}}
static ssize_t try_destroy(machine_t *machine){ // {{{
	try_userdata      *userdata = (try_userdata *)machine->userdata;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&userdata->machine, &r_free);
	
	free(userdata->try_end->userdata);
	free(userdata->try_end);
	free(userdata);
	return 0;
} // }}}
static ssize_t try_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	try_userdata          *userdata          = (try_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->request,     config, HK(request));
	if(ret == 0){
		userdata->request_out = userdata->request;
	}
	hash_data_get(ret, TYPE_HASHKEYT, userdata->request_out, config, HK(request_out));
	
	hash_holder_consume(ret, userdata->machine, config, HK(shop));
	if(ret != 0)
		return error("shop parameter not supplied");
	
	return 0;
} // }}}

static ssize_t get_hash(data_t *data, data_t *freeme, hash_t **hash){ // {{{
	ssize_t                ret;
	
	data_realholder(ret, data, data);
	if(ret < 0)
		return ret;
	
	// plain hash
	if(data->type == TYPE_HASHT){
		*hash = (hash_t *)data->ptr;
		return 0;
	}
	
	// not hash, try convert
	freeme->type = TYPE_HASHT;
	freeme->ptr  = NULL;
	
	fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, data, FORMAT(packed) };
	if( (ret = data_query(freeme, &r_convert)) < 0)
		return ret;
	
	*hash = (hash_t *)freeme->ptr;
	return 0;
} // }}}

static ssize_t try_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t               ret;
	data_t                freeme;
	request_t            *try_request;
	try_userdata         *userdata          = (try_userdata *)machine->userdata;
	tryend_userdata      *te_userdata       = userdata->try_end->userdata;
	
	te_userdata->machine  = machine;
	te_userdata->request  = request;
	te_userdata->ret      = 0;
	
	data_set_void(&freeme);
	
	if(userdata->request == 0){
		try_request = request;
	}else{
		if( (ret = get_hash(hash_data_find(request, userdata->request), &freeme, &try_request)) < 0)
			return ret;
	}
	
	request_enter_context(request);
	
		stack_call(userdata->try_end);

			fastcall_query r_query = { { 3, ACTION_QUERY }, try_request };
			ret = data_query(&userdata->machine, &r_query);
		
		stack_clean();

		if(ret < 0){
			if(userdata->request == 0){
				request_t r_pass[] = {
					{ HK(ret), DATA_SIZET(0) },
					hash_inline(request),
					hash_end
				};
				te_userdata->ret = machine_pass(machine, r_pass);
			}else{
				request_t r_pass[] = {
					{ HK(ret), DATA_SIZET(0) },
					hash_inline(try_request),
					hash_end
				};
				
				request_t r_next[] = {
					{ HK(ret),               DATA_SIZET(0)   },
					{ userdata->request_out, DATA_PTR_HASHT(r_pass) },
					hash_inline(request),
					hash_end
				};
				te_userdata->ret = machine_pass(machine, r_next);
			}
		}
	
	request_leave_context();
	
	data_free(&freeme);
	return te_userdata->ret;
} // }}}

machine_t try_proto = {
	.class          = "shop/try",
	.supported_api  = API_HASH,
	.func_init      = &try_init,
	.func_destroy   = &try_destroy,
	.func_configure = &try_configure,
	.machine_type_hash = {
		.func_handler = &try_handler
	}
};

