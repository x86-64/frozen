#include <libfrozen.h>

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
 *              return_to               = (hashkey_t)"return_to"  // return_to hash key, default "return_to"
 * }
 * @endcode
 */

#define EMODULE 54

typedef struct try_userdata {
	data_t                 machine;
	hashkey_t              request;
	hashkey_t              request_out;
	hashkey_t              return_to;
	machine_t             *try_end;
	
	thread_data_ctx_t      thread_data;
} try_userdata;

typedef struct try_threaddata {
	machine_t             *machine;
	request_t             *request;
	ssize_t                ret;
} try_threaddata;

static ssize_t try_end_handler(machine_t *machine, request_t *request){ // {{{
	try_userdata         *userdata          = (try_userdata *)machine->userdata;
	try_threaddata       *threaddata        = thread_data_get(&userdata->thread_data);
	
	if(userdata->request == 0){
		threaddata->ret = machine_pass(threaddata->machine, request);
	}else{
		request_t r_next[] = {
			{ userdata->request_out, DATA_PTR_HASHT(request) },
			hash_inline(threaddata->request),
			hash_end
		};
		threaddata->ret = machine_pass(threaddata->machine, r_next);
	}
	return 0;
} // }}}

machine_t try_end_proto = {
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &try_end_handler
	}
};

static void * try_threaddata_create(try_userdata *userdata){ // {{{
	try_threaddata      *threaddata;
	
	if( (threaddata = malloc(sizeof(try_threaddata))) == NULL)
		goto error1;
	
	return threaddata;

error1:
	return NULL;
} // }}}
static void   try_threaddata_destroy(try_threaddata *threaddata){ // {{{
	free(threaddata);
} // }}}

static ssize_t try_init(machine_t *machine){ // {{{
	try_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(try_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->request     = 0; 
	userdata->request_out = 0; 
	userdata->return_to   = HK(return_to);
	
	if((userdata->try_end = malloc(sizeof(machine_t))) == NULL){
		free(userdata);
		return error("malloc failed");
	}
	memcpy(userdata->try_end, &try_end_proto, sizeof(machine_t));
	userdata->try_end->userdata = userdata;
	
	return thread_data_init(
		&userdata->thread_data, 
		(f_thread_create)&try_threaddata_create,
		(f_thread_destroy)&try_threaddata_destroy,
		userdata);
} // }}}
static ssize_t try_destroy(machine_t *machine){ // {{{
	try_userdata      *userdata = (try_userdata *)machine->userdata;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&userdata->machine, &r_free);
	
	thread_data_destroy(&userdata->thread_data);
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
	hash_data_get(ret, TYPE_HASHKEYT, userdata->return_to,   config, HK(return_to));
	
	hash_holder_consume(ret, userdata->machine, config, HK(shop));
	if(ret != 0)
		return error("shop parameter not supplied");
	
	return 0;
} // }}}

static ssize_t get_hash(data_t *data, data_t *freeme, hash_t **hash){ // {{{
	ssize_t                ret;
	
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( (ret = data_query(data, &r_getdata)) < 0)
		return ret;
	
	// plain hash
	if(r_getdata.data->type == TYPE_HASHT){
		*hash = (hash_t *)r_getdata.data->ptr;
		return 0;
	}
	
	// not hash, try convert
	freeme->type = TYPE_HASHT;
	freeme->ptr  = NULL;
	
	fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, r_getdata.data, FORMAT(binary) };
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
	try_threaddata       *threaddata        = thread_data_get(&userdata->thread_data);
	
	request_set_current(request);
	
	threaddata->machine  = machine;
	threaddata->request  = request;
	threaddata->ret      = 0;
	
	data_set_void(&freeme);
	
	if(userdata->request == 0){
		try_request = request;
	}else{
		if( (ret = get_hash(hash_data_find(request, userdata->request), &freeme, &try_request)) < 0)
			return ret;
	}
	
	request_t r_next[] = {
		{ userdata->return_to, DATA_MACHINET(userdata->try_end) },
		hash_inline(try_request),
		hash_end
	};
	
	fastcall_query r_query = { { 3, ACTION_QUERY }, r_next };
	if( (ret = data_query(&userdata->machine, &r_query)) < 0){
		if(userdata->request == 0){
			request_t r_pass[] = {
				{ HK(ret), DATA_PTR_SIZET(&ret) },
				hash_inline(request),
				hash_end
			};
			threaddata->ret = machine_pass(machine, r_pass);
		}else{
			request_t r_pass[] = {
				{ HK(ret), DATA_PTR_SIZET(&ret) },
				hash_inline(try_request),
				hash_end
			};
			
			request_t r_next[] = {
				{ HK(ret),               DATA_PTR_SIZET(&ret)   },
				{ userdata->request_out, DATA_PTR_HASHT(r_pass) },
				hash_inline(request),
				hash_end
			};
			threaddata->ret = machine_pass(machine, r_next);
		}
	}
	
	data_free(&freeme);
	return threaddata->ret;
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

