#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_rebuild machine/rebuild
 */
/**
 * @ingroup mod_machine_rebuild
 * @page page_rebuild_info Description
 *
 * This module rebuild shop when underlying machine turns out unconsistent and return special error code
 */
/**
 * @ingroup mod_machine_rebuild
 * @page page_rebuild_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "machine/rebuild",
 *              enum_method             = 
 *                                        "iterate"           # count elements and read all of them from 0 to <count>
 *                                        "cursor"            # create cursor at starting position and start iterate
 *              retry_request           = (uint_t)'1'         # rerun request with triggered rebuild after rebuild, default 1
 *              req_rebuild             = { ... }             # custom rebuild request
 *              req_rebuild_enable      = (uint_t)'1'         # enable\disable rebuild request at all, default 1
 *              req_rebuild_destination = "far_index"         # set rebuild request destination machine
 *              
 *              # options for 'iterate' method
 *                 offset_key  = "offset",                    # key with with would be current index requested
 *                 req_count   = { ... }                      # additional parameters for count request
 *                 req_read    = { ... }                      # additional parameters for read request (eg. buffer)
 * }
 * @endcode
 */
/**
 * @ingroup mod_machine_rebuild
 * @page page_rebuild_io Input and output
 * 
 * Module by default pass all requests to underlying machine unchanged.
 * If return code equal -EBADF, rebuild started.
 *
 * @li Enum 'iterate' count request
 * @return 0:      request ok
 * @return -<ANY>  request failed
 * 
 * @li Enum 'iterate' read request
 * @return 0:      request ok
 * @return -EBADF: request failed, shop is again in unconsistend state, starting rebuild again
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

typedef struct rebuildmon_userdata {
	machine_t             *reader;
	machine_t             *writer;
	uintmax_t              retry_request;
} rebuildmon_userdata;

typedef struct rebuildread_userdata {
	enum_method            enum_method;
	hash_t                *req_rebuild;
	hash_t                *req_count;
	hash_t                *req_read;
	hashkey_t              hk_offset;
} rebuildread_userdata;

static enum_method  rebuild_string_to_method(char *str){ // {{{
	if(str != NULL){
		if(strcasecmp(str, "iterate") == 0) return ENUM_COUNT_AND_READ_ITERATE;
		if(strcasecmp(str, "cursor")  == 0) return ENUM_CURSOR_ITERATE;
	}
	return ENUM_DEFAULT;
} // }}}
static ssize_t rebuild_rebuild(machine_t *reader, machine_t *writer){ // {{{
	ssize_t                ret;
	rebuildread_userdata  *userdata          = (rebuildread_userdata *)reader->userdata;
	//uintmax_t              rebuild_num = 0;
	
	if(strcmp(reader->class, "machine/rebuild_reader") != 0)
		return error("not reader supplied");
	
redo:
	//if(rebuild_num++ >= userdata->max_rebuilds)
	//	return error("rebuild max rebuilds reached");
	
	if(userdata->req_rebuild != NULL){
		// NOTE rebuild request must return -EBADF as good result
		if( (ret = machine_query(writer, userdata->req_rebuild)) != -EBADF)
			return ret;
	}
	
	switch(userdata->enum_method){
		case ENUM_COUNT_AND_READ_ITERATE:;
			uintmax_t              i;
			uintmax_t              count = 0;
			
			request_t r_count[] = {
				{ HK(action), DATA_ACTIONT(ACTION_COUNT)      },
				{ HK(buffer), DATA_PTR_UINTT(&count)          },
				hash_next(userdata->req_count)
			};
			//if( (ret = machine_pass(reader, r_count)) < 0 )
			//	return ret;
			machine_pass(reader, r_count); // ignore struct_unpack errors here..
			
			for(i=0; i<count; i++){
				request_t r_read[] = {
					{ HK(action),          DATA_ACTIONT(ACTION_READ)          },
					{ userdata->hk_offset, DATA_UINTT(i)                      }, // copy of i, not ptr
					hash_next(userdata->req_read)
				};
				ret = machine_pass(reader, r_read);
				if( ret == -EBADF )
					goto redo;   // start from beginning
				if( ret < 0 ){
					log_error("rebuild error - read: %d: %s\n", ret, describe_error(ret));
					return ret;
				}
				
				/*uintmax_t offset;
				request_t r_create[] = {
					{ HK(action),          DATA_ACTIONT(ACTION_CREATE)        },
					{ userdata->hk_offset, DATA_PTR_UINTT(&offset)            },
					hash_next(r_read)
				};
				ret = machine_query(writer, r_create);
				if( ret == -EBADF )
					goto redo;
				if(ret < 0)
					return ret;*/
				
				request_t r_write[] = {
					{ HK(action),          DATA_ACTIONT(ACTION_WRITE)         },
					{ userdata->hk_offset, DATA_UINTT(i)                      },
					hash_next(r_read)
				};
				ret = machine_query(writer, r_write);
				if( ret == -EBADF )
					goto redo;
				if( ret < 0 ){
					log_error("rebuild error - write: %d: %s\n", ret, describe_error(ret));
					return ret;
				}
			}
			break;
		case ENUM_CURSOR_ITERATE:
			// TODO actual code
			break;
	};
	return 0;
} // }}}

static int rebuildread_init(machine_t *machine){ // {{{
	rebuildread_userdata  *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(rebuildread_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->hk_offset = HK(offset);
	return 0;
} // }}}
static int rebuildread_destroy(machine_t *machine){ // {{{
	rebuildread_userdata  *userdata          = (rebuildread_userdata *)machine->userdata;
	
	hash_free(userdata->req_rebuild);
	hash_free(userdata->req_count);
	hash_free(userdata->req_read);

	free(userdata);
	return 0;
} // }}}
static int rebuildread_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              req_rebuild_enable= 1;
	char                  *req_rebuild_dest  = NULL;
	hash_t                *req_rebuild       = NULL;
	hash_t                *req_count         = NULL;
	hash_t                *req_read          = NULL;
	char                  *enum_method_str   = NULL;
	rebuildread_userdata  *userdata          = (rebuildread_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_STRINGT,  enum_method_str,                config, HK(enum_method));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->hk_offset,            config, HK(offset_key));
	hash_data_get(ret, TYPE_UINTT,   req_rebuild_enable,              config, HK(req_rebuild_enable));
	hash_data_get(ret, TYPE_STRINGT, req_rebuild_dest,                config, HK(req_rebuild_destination));
	hash_data_consume(ret, TYPE_HASHT,   req_count,                       config, HK(req_count));
	hash_data_consume(ret, TYPE_HASHT,   req_read,                        config, HK(req_read));
	hash_data_consume(ret, TYPE_HASHT,   req_rebuild,                     config, HK(req_rebuild));
	
	if(req_rebuild_enable != 0){                         // emit allowed, prepare signal
		hash_t r_signal_orig[] = {
			{ HK(action),        DATA_ACTIONT(ACTION_REBUILD)           },
			hash_end	
		};
		if(req_rebuild == NULL)                      // no custom signal supplied, use standard
			req_rebuild = r_signal_orig;
		
		if(req_rebuild_dest != NULL){                 // destination supplied, append to hash
			hash_t r_signal_dest[] = {
				{ HK(destination),   DATA_PTR_STRING(req_rebuild_dest) },
				hash_next(req_rebuild)
			};
			userdata->req_rebuild = hash_copy(r_signal_dest);
		}else{
			userdata->req_rebuild = hash_copy(req_rebuild);
		}
	}
	
	userdata->enum_method = rebuild_string_to_method(enum_method_str);
	userdata->req_count   = req_count;
	userdata->req_read    = req_read;

	return 0;
} // }}}

machine_t rebuild_reader_proto = {
	.class          = "machine/rebuild_reader",
	.supported_api  = API_HASH,
	.func_init      = &rebuildread_init,
	.func_destroy   = &rebuildread_destroy,
	.func_configure = &rebuildread_configure,
};

machine_t rebuild_writer_proto = {
	.class          = "machine/rebuild_writer",
	.supported_api  = API_HASH,
};

static int rebuildmon_init(machine_t *machine){ // {{{
	rebuildmon_userdata   *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(rebuildmon_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->retry_request = 1;
	return 0;
} // }}}
static int rebuildmon_destroy(machine_t *machine){ // {{{
	rebuildmon_userdata   *userdata          = (rebuildmon_userdata *)machine->userdata;
	
	shop_destroy(userdata->reader);
	shop_destroy(userdata->writer);
	
	free(userdata);
	return 0;
} // }}}
static int rebuildmon_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	rebuildmon_userdata   *userdata          = (rebuildmon_userdata *)machine->userdata;
	
	hash_data_get    (ret, TYPE_UINTT,     userdata->retry_request,       config, HK(retry_request));
	hash_data_consume(ret, TYPE_MACHINET,  userdata->writer,              config, HK(writer));
	hash_data_consume(ret, TYPE_MACHINET,  userdata->reader,              config, HK(reader));
	if(ret != 0)
		return error("no reader supplied");
	
	return 0;
} // }}}
static ssize_t rebuildmon_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	rebuildmon_userdata   *userdata          = (rebuildmon_userdata *)machine->userdata;

retry:;
	ret = machine_pass(machine, request);
	if(ret == -EBADF){
		if(userdata->writer == NULL){
			if(machine->cnext){
				userdata->writer = machine->cnext;
			}else{
				return error("no writer exists");
			}
		}
		if( (ret = rebuild_rebuild(userdata->reader, userdata->writer)) < 0)
			return ret;
		
		if(userdata->retry_request != 0)
			goto retry;
	}
	return ret;
} // }}}

machine_t rebuild_monitor_proto = {
	.class          = "machine/rebuild_monitor",
	.supported_api  = API_HASH,
	.func_init      = &rebuildmon_init,
	.func_destroy   = &rebuildmon_destroy,
	.func_configure = &rebuildmon_configure,
	.machine_type_hash = {
		.func_handler = &rebuildmon_handler
	}
};

