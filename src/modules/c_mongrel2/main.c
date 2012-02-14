#include <libfrozen.h>

#include <errors_list.c>

/**
 * @ingroup machine
 * @addtogroup mod_machine_mongrel2 module/c_mongrel2
 */
/**
 * @ingroup mod_machine_mongrel2
 * @page page_mongrel2_info Description
 *
 * This module implement Mongrel2 protocol parsers. It works in pair with ZeroMQ module. c_mongrel2_parse parse mongrel2 request
 * and emit it to underlying machines. c_mongrel2_reply consturct reply for mongrel server and pass it to underlying backned.
 */
/**
 * @ingroup mod_machine_mongrel2
 * @page page_mongrel2_config Configuration
 *  
 * Accepted configuration for parser machine:
 * @code
 * {
 *              class                   = "modules/c_mongrel2_parse"
 * }
 * @endcode
 * 
 * Accepted configuration for reply machine
 * @code
 * {
 *              class                   = "modules/c_mongrel2_reply",
 *              buffer                  = (hashkey_t)'buffer',                 // output buffer hashkey name, default "buffer"
 *              body                    = (hashkey_t)'body',                   // input http request body hashkey name, default "body"
 *              close                   = (uint_t)'0',                         // close connection after reply, default 1
 *              lazy                    = (uint_t)'1'                          // lazy packing (pass structure instead of ready-to-use buffer), default 0
 * }
 * @endcode
 */
/**
 * @ingroup mod_machine_mongrel2
 * @page page_mongrel2_io Input and output
 * 
 * c_mongrel2_parse expect fast ACTION_WRITE request as input and emit following hash request on output:
 * @code
 * {
 *              action                  = (action_t)"write",
 *              uuid                    = (raw_t)"<server uuid>",              // server uuid, use it for reply
 *              clientid                = (raw_t)"<clientid>",                 // mongrel client connection id
 *              path                    = (raw_t)"/",                          // request url
 *              headers                 = (raw_t)"{<json headers>}",           // request headers in form of json string, not parsed
 *              body                    = (raw_t)"<body>"                      // request body
 * }
 * @endcode
 *
 * c_mongrel2_reply expect following minimum request for input:
 * @code
 * {
 *              uuid                    = (raw_t)"<server uuid>",              // server uuid to reply for
 *              clinetid                = (raw_t)"<clientid>",                 // mongrel client connection id, or list of id's
 *             [config->body]           =                                      // reply for client
 *                                        (raw_t)"<body>"                      // - full request body, including "HTTP/1.1 ..."
 *                                        (void_t)""                           // - ask mongrel to close connection
 * } 
 * @endcode
 * c_mongrel2_reply emit following request:
 * @code
 * {
 *             [config->buffer]         = (struct_t)"",                        // constructed reply
 *             ...
 * }
 * @endcode
 */

typedef struct mongrel2_userdata {
	hashkey_t              buffer;
	hashkey_t              body;
	uintmax_t              close_conn;
	uintmax_t              lazy;
} mongrel2_userdata;

static int mongrel2_init(machine_t *machine){ // {{{
	mongrel2_userdata     *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(mongrel2_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->buffer     = HK(buffer);
	userdata->body       = HK(body);
	userdata->close_conn = 1;
	return 0;
} // }}}
static int mongrel2_destroy(machine_t *machine){ // {{{
	mongrel2_userdata     *userdata          = (mongrel2_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int mongrel2_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	mongrel2_userdata     *userdata          = (mongrel2_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->buffer,     config, HK(buffer));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->body,       config, HK(body));
	hash_data_get(ret, TYPE_UINTT,    userdata->close_conn, config, HK(close));
	hash_data_get(ret, TYPE_UINTT,    userdata->lazy,       config, HK(lazy));
	return 0;
} // }}}

static ssize_t send_reply(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	mongrel2_userdata     *userdata          = (mongrel2_userdata *)machine->userdata;
	
	hash_t reply_struct[] = {
		{ HK(uuid), DATA_HASHT(
			{ HK(format),  DATA_FORMATT(FORMAT(clean))     },
			hash_end
		)},
		{ 0, DATA_HASHT(
			{ HK(format),  DATA_FORMATT(FORMAT(clean))     },
			{ HK(default), DATA_STRING(" ")                },
			hash_end
		)},
		{ HK(clientid), DATA_HASHT(
			{ HK(format),  DATA_FORMATT(FORMAT(netstring)) },
			hash_end
		)},
		{ 0, DATA_HASHT(
			{ HK(format),  DATA_FORMATT(FORMAT(clean))     },
			{ HK(default), DATA_STRING(" ")                },
			hash_end
		)},
		{ userdata->body, DATA_HASHT(
			{ HK(format),  DATA_FORMATT(FORMAT(clean))     },
			hash_end
		)},
		hash_end
	};
	
	if(userdata->lazy != 0){
		request_t r_reply[] = {
			{ userdata->buffer, DATA_STRUCTT(reply_struct, request) },
			hash_inline(request),
			hash_end
		};
		if( (ret = machine_pass(machine, r_reply)) < 0 )
			return ret;
	}else{
		data_t                 d_struct          = DATA_STRUCTT(reply_struct, request);
		data_t                 d_raw             = { TYPE_RAWT, NULL };
		
		fastcall_convert_to r_convert = {
			{ 4, ACTION_CONVERT_TO },
			&d_raw,
			FORMAT(clean)
		};
		if( (ret = data_query(&d_struct, &r_convert)) < 0)
			return ret;
		
		request_t r_reply[] = {
			{ userdata->buffer, d_raw },
			hash_inline(request),
			hash_end
		};
		ret = machine_pass(machine, r_reply);
		
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&r_reply[0].data, &r_free);           // buffer can be consumed
	}
	return ret;
} // }}}

static ssize_t mongrel2_reply_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	mongrel2_userdata     *userdata          = (mongrel2_userdata *)machine->userdata;
	
	if( (ret = send_reply(machine, request)) < 0)
		return ret;
	
	if(userdata->close_conn == 1){
		request_t r_voidbody[] = {
			{ userdata->body, DATA_VOID },
			hash_inline(request),
			hash_end
		};
		if( (ret = send_reply(machine, r_voidbody)) < 0)
			return ret;
	}
	return 0;
} // }}}

static machine_t mongrel2_reply_proto = {
	.class          = "modules/c_mongrel2_reply",
	.supported_api  = API_HASH,
	.func_init      = &mongrel2_init,
	.func_destroy   = &mongrel2_destroy,
	.func_configure = &mongrel2_configure,
	.machine_type_hash = {
		.func_handler = &mongrel2_reply_handler
	},
};

static ssize_t mongrel2_parse_handler(machine_t *machine, request_t *request){ // {{{
	data_t                *buffer;
	char                  *p, *uuid_p, *connid_p, *path_p, *header_p, *body_p, *e;
	uintmax_t                  uuid_l,  connid_l,  path_l,  header_l,  body_l;
	
	if( (buffer = hash_data_find(request, HK(buffer))) == NULL )
		return -EINVAL;
	
	// little hack here, parsing real data would be difficult, so we accept only "plain" memory chunks
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	fastcall_length      r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(clean) };
	if( data_query(buffer, &r_len) != 0 || data_query(buffer, &r_ptr) != 0 || r_ptr.ptr == NULL)
		return -EINVAL;
	
	p = r_ptr.ptr;
	e = r_ptr.ptr + r_len.length;
	
	     uuid_p   = p;
	if( (connid_p = memchr(p, ' ', (e-p))) == NULL) return -EINVAL; uuid_l   = connid_p - uuid_p;   p = ++connid_p;
	if( (path_p   = memchr(p, ' ', (e-p))) == NULL) return -EINVAL; connid_l = path_p   - connid_p; p = ++path_p;
	if( (header_p = memchr(p, ' ', (e-p))) == NULL) return -EINVAL; path_l   = header_p - path_p;
	
	header_p++;
	header_l = 0; sscanf(header_p, "%" SCNuMAX, &header_l);
	if( (header_p = memchr(header_p, ':', (e-header_p))) == NULL) return -EINVAL; header_p++;
	if(header_l >= (e-header_p)) return -EINVAL;
	
	body_p = header_p + header_l + 1;
	body_l   = 0; sscanf(body_p,   "%" SCNuMAX, &body_l);
	if( (body_p   = memchr(body_p,   ':', (e-body_p))) == NULL) return -EINVAL; body_p++;
	if(body_l >= (e-body_p)) return -EINVAL;
	
	request_t r_next[] = {
		{ HK(uuid),     DATA_RAW(uuid_p,   uuid_l)   },
		{ HK(clientid), DATA_RAW(connid_p, connid_l) },
		{ HK(path),     DATA_RAW(path_p,   path_l)   },
		{ HK(headers),  DATA_RAW(header_p, header_l) },
		{ HK(body),     DATA_RAW(body_p,   body_l)   },
		hash_inline(request),
		hash_end
	};
	
	return machine_pass(machine, r_next);
} // }}}

static machine_t mongrel2_parse_proto = {
	.class          = "modules/c_mongrel2_parse",
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &mongrel2_parse_handler,
	}
};

int main(void){
	errors_register((err_item *)&errs_list, &emodule);
	class_register(&mongrel2_reply_proto);
	class_register(&mongrel2_parse_proto);
	return 0;
}
