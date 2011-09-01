#include <libfrozen.h>

#define EMODULE 24

typedef enum enum_method {
	ENUM_COUNT_AND_READ_ITERATE,
	ENUM_CURSOR_ITERATE,           // TODO

	ENUM_DEFAULT = ENUM_COUNT_AND_READ_ITERATE
} enum_method;

typedef struct rebuild_userdata {
	enum_method            enum_method;
	hash_t                *rebuild_signal;
	uintmax_t              requery_after_rebuild;
} rebuild_userdata;

static enum_method  rebuild_string_to_method(char *str){ // {{{
	if(str != NULL){
		if(strcasecmp(str, "iterate") == 0) return ENUM_COUNT_AND_READ_ITERATE;
		if(strcasecmp(str, "cursor")  == 0) return ENUM_CURSOR_ITERATE;
	}
	return ENUM_DEFAULT;
} // }}}
static ssize_t rebuild_rebuild(backend_t *backend){ // {{{
	ssize_t                ret;
	//uint64_t               keyid;
	off_t                  i;
	off_t                  count = 0;
	//uintmax_t              rebuild_num = 0;
	//char                  *buffer;
	//rebuild_userdata      *userdata = (rebuild_userdata *)backend->userdata;
	
	
	// prepare buffer
	//if( (buffer = malloc(userdata->buffer_size)) == NULL)
	//	return error("malloc failed");
	
//redo:
	//if(rebuild_num++ >= userdata->max_rebuilds)
	//	return error("rebuild max rebuilds reached");

	//if( (ret = userdata->rebuild_proto->func_rebuild(&userdata->rebuild, count)) < 0)
	//	return ret;
	
	
	// count elements
	request_t r_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT) },
		{ HK(buffer), DATA_PTR_OFFT(&count)           },
		{ HK(ret),    DATA_PTR_SIZET(&ret)            },
		hash_end
	};
	if(backend_pass(backend, r_count) < 0 || ret < 0)
		return 0;
	
	// process items
	for(i=0; i<count; i++){
	//	keyid = 0;
		
		request_t r_read[] = {
			{ HK(action),          DATA_UINT32T(ACTION_CRWD_READ)          },
	//		{ userdata->key_from,  DATA_PTR_UINT64T(&keyid)                },
	//		{ userdata->key_to,    DATA_OFFT(i)                            },
	//		{ HK(buffer),          DATA_RAW(buffer, userdata->buffer_size) },
	//		{ HK(size),            DATA_SIZET(userdata->buffer_size)       },
			{ HK(ret),             DATA_PTR_SIZET(&ret)                    },
			hash_end
		};
		if( backend_pass(backend, r_read) < 0 || ret < 0){
			ret = error("failed call\n");
			break;
		}
	}
	
	//free(buffer);
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
	uintmax_t              rebuild_sig_emit  = 1;
	char                  *rebuild_sig_name  = NULL;
	hash_t                *rebuild_signal    = NULL;
	char                  *enum_method_str   = NULL;
	rebuild_userdata      *userdata          = (rebuild_userdata *)backend->userdata;
	
	userdata->requery_after_rebuild = 1;
	
	hash_data_copy(ret, TYPE_STRINGT, enum_method_str,                 config, HK(enum_method));
	hash_data_copy(ret, TYPE_UINTT,   userdata->requery_after_rebuild, config, HK(requery_after_rebuild));
	
	hash_data_copy(ret, TYPE_UINTT,   rebuild_sig_emit,                config, HK(sig_rebuild));
	hash_data_copy(ret, TYPE_HASHT,   rebuild_signal,                  config, HK(sig_rebuild_hash));
	hash_data_copy(ret, TYPE_STRINGT, rebuild_sig_name,                config, HK(sig_rebuild_destination));
	if(rebuild_sig_emit != 0){                           // emit allowed, prepare signal
		hash_t r_signal_orig[] = {
			{ HK(action),        DATA_UINT32T(ACTION_REBUILD)           },
			hash_end	
		};
		if(rebuild_signal == NULL)                   // no custom signal supplied, use standard
			rebuild_signal = r_signal_orig;
		
		hash_t r_signal_dest[] = {
			{ HK(destination),   DATA_PTR_STRING_AUTO(rebuild_sig_name) },
			hash_next(rebuild_signal)
		};
		if(rebuild_sig_name != NULL)                 // destination supplied, append to hash
			rebuild_signal = r_signal_dest;
		
		userdata->rebuild_signal = hash_copy(rebuild_signal);
	}
	
	userdata->enum_method = rebuild_string_to_method(enum_method_str);

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
			if(userdata->requery_after_rebuild != 0)
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

