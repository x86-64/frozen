#include <libfrozen.h>

/**
 * @ingroup modules
 * @addtogroup mod_backend_rebuild Module 'backend/rebuild'
 */
/**
 * @ingroup mod_backend_rebuild
 * @page page_rebuild_info Description
 *
 * This module rebuild chain when underlying backend turns out unconsistent and return special error code
 */
/**
 * @ingroup mod_backend_rebuild
 * @page page_rebuild_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "backend/rebuild",
 *              enum_method             = 
 *                                        "iterate"           # count elements and read all of them from 0 to <count>
 *                                        "cursor"            # create cursor at starting position and start iterate
 *              retry_request           = (uint_t)'1'         # rerun request with triggered rebuild after rebuild, default 1
 *              req_rebuild             = { ... }             # custom rebuild request
 *              req_rebuild_enable      = (uint_t)'1'         # enable\disable rebuild request at all, default 1
 *              req_rebuild_destination = "far_index"         # set rebuild request destination backend
 *              
 *              # options for 'iterate' method
 *                 offset_key  = "offset",                    # key with with would be current index requested
 *                 req_count   = { ... }                      # additional parameters for count request
 *                 req_read    = { ... }                      # additional parameters for read request (eg. buffer)
 * }
 * @endcode
 */
/**
 * @ingroup mod_backend_rebuild
 * @page page_rebuild_io Input and output
 * 
 * Module by default pass all requests to underlying backend unchanged.
 * If return code equal -EBADF, rebuild started.
 *
 * @li Enum 'iterate' count request
 * @return 0:      request ok
 * @return -<ANY>  request failed
 * 
 * @li Enum 'iterate' read request
 * @return 0:      request ok
 * @return -EBADF: request failed, chain is again in unconsistend state, starting rebuild again
 * @return -<ANY>  request failed
 * 
 * @li Rebuild request
 * @return -EBADF: request ok
 * @return -<ANY>: request failed
 * 
 */

#define EMODULE 24

typedef enum enum_method {
	ENUM_COUNT_AND_READ_ITERATE,
	ENUM_CURSOR_ITERATE,

	ENUM_DEFAULT = ENUM_COUNT_AND_READ_ITERATE
} enum_method;

typedef struct rebuild_userdata {
	enum_method            enum_method;
	uintmax_t              retry_request;
	hash_t                *req_rebuild;
	hash_t                *req_count;
	hash_t                *req_read;
	hash_key_t             hk_offset;
} rebuild_userdata;

static enum_method  rebuild_string_to_method(char *str){ // {{{
	if(str != NULL){
		if(strcasecmp(str, "iterate") == 0) return ENUM_COUNT_AND_READ_ITERATE;
		if(strcasecmp(str, "cursor")  == 0) return ENUM_CURSOR_ITERATE;
	}
	return ENUM_DEFAULT;
} // }}}
static ssize_t rebuild_rebuild(backend_t *backend){ // {{{
	ssize_t                ret, rret;
	rebuild_userdata      *userdata = (rebuild_userdata *)backend->userdata;
	//uintmax_t              rebuild_num = 0;
	
redo:
	//if(rebuild_num++ >= userdata->max_rebuilds)
	//	return error("rebuild max rebuilds reached");
	
	if(userdata->req_rebuild != NULL){
		if( (ret = backend_pass(backend, userdata->req_rebuild)) != -EBADF) // NOTE rebuild request must return -EBADF as good result
			return ret;
	}
	
	switch(userdata->enum_method){
		case ENUM_COUNT_AND_READ_ITERATE:;
			uintmax_t              i;
			uintmax_t              count = 0;
			
			request_t r_count[] = {
				{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT) },
				{ HK(buffer), DATA_PTR_UINTT(&count)          },
				{ HK(ret),    DATA_PTR_SIZET(&rret)           },
				hash_next(userdata->req_count)
			};
			if( (ret = backend_pass(backend, r_count)) < 0 )
				return ret;
			if( rret < 0 )
				return rret;
			
			for(i=0; i<count; i++){
				request_t r_read[] = {
					{ HK(action),          DATA_UINT32T(ACTION_CRWD_READ)          },
					{ userdata->hk_offset, DATA_UINTT(i)                           }, // copy of i, not ptr
					{ HK(ret),             DATA_PTR_SIZET(&rret)                   },
					hash_next(userdata->req_read)
				};
				if( (ret = backend_pass(backend, r_read)) < 0 )
					return ret;
				if( rret == -EBADF )
					goto redo;   // start from beginning
				if( rret < 0 )
					return rret;
			}
			break;
		case ENUM_CURSOR_ITERATE:
			// TODO actual code
			break;
	};
	return 0;
} // }}}

static int rebuild_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(rebuild_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int rebuild_destroy(backend_t *backend){ // {{{
	rebuild_userdata      *userdata = (rebuild_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int rebuild_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              req_rebuild_enable= 1;
	char                  *req_rebuild_dest  = NULL;
	hash_t                *req_rebuild       = NULL;
	hash_t                *req_count         = NULL;
	hash_t                *req_read          = NULL;
	char                  *enum_method_str   = NULL;
	char                  *hk_offset_str     = NULL;
	rebuild_userdata      *userdata          = (rebuild_userdata *)backend->userdata;
	
	userdata->retry_request = 1;
	
	hash_data_copy(ret, TYPE_UINTT,   userdata->retry_request,         config, HK(retry_request));
	hash_data_copy(ret, TYPE_STRINGT, enum_method_str,                 config, HK(enum_method));
	hash_data_copy(ret, TYPE_STRINGT, hk_offset_str,                   config, HK(offset_key));
	hash_data_copy(ret, TYPE_HASHT,   req_count,                       config, HK(req_count));
	hash_data_copy(ret, TYPE_HASHT,   req_read,                        config, HK(req_read));
	hash_data_copy(ret, TYPE_HASHT,   req_rebuild,                     config, HK(req_rebuild));
	hash_data_copy(ret, TYPE_UINTT,   req_rebuild_enable,              config, HK(req_rebuild_enable));
	hash_data_copy(ret, TYPE_STRINGT, req_rebuild_dest,                config, HK(req_rebuild_destination));
	
	if(req_rebuild_enable != 0){                         // emit allowed, prepare signal
		hash_t r_signal_orig[] = {
			{ HK(action),        DATA_UINT32T(ACTION_REBUILD)           },
			hash_end	
		};
		if(req_rebuild == NULL)                      // no custom signal supplied, use standard
			req_rebuild = r_signal_orig;
		
		hash_t r_signal_dest[] = {
			{ HK(destination),   DATA_PTR_STRING_AUTO(req_rebuild_dest) },
			hash_next(req_rebuild)
		};
		if(req_rebuild_dest != NULL)                 // destination supplied, append to hash
			req_rebuild = r_signal_dest;
		
		userdata->req_rebuild = hash_copy(req_rebuild);
	}
	
	userdata->enum_method = rebuild_string_to_method(enum_method_str);
	userdata->hk_offset   = hash_string_to_key(hk_offset_str);
	userdata->req_count   = hash_copy(req_count);
	userdata->req_read    = hash_copy(req_read);

	return 0;
} // }}}

static ssize_t rebuild_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret1, ret2;
	rebuild_userdata      *userdata = (rebuild_userdata *)backend->userdata;

retry:;
	request_t  r_ret[] = {
		{ HK(ret), DATA_PTR_SIZET(&ret2) },
		hash_next(request)
	};
	
	if( (ret1 = backend_pass(backend, r_ret)) < 0 ){
		return ret1;
	}else{
		if(ret2 == -EBADF){
			rebuild_rebuild(backend);
			if(userdata->retry_request != 0)
				goto retry;
		}
		return ret2;
	}
} // }}}

backend_t rebuild_proto = {
	.class          = "backend/rebuild",
	.supported_api  = API_HASH,
	.func_init      = &rebuild_init,
	.func_destroy   = &rebuild_destroy,
	.func_configure = &rebuild_configure,
	.backend_type_hash = {
		.func_handler = &rebuild_handler
	}
};

