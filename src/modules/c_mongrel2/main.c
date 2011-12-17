#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_mongrel2 module/c_mongrel2
 */
/**
 * @ingroup mod_backend_mongrel2
 * @page page_mongrel2_info Description
 *
 * This module implement Mongrel2 protocol parsers. It works in pair with c_zmq module.
 */
/**
 * @ingroup mod_backend_mongrel2
 * @page page_mongrel2_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "modules/c_mongrel2_parse"
 * }
 * @endcode
 * 
 * @code
 * {
 *              class                   = "modules/c_mongrel2_reply"
 * }
 * @endcode
 */

#define EMODULE 102

static ssize_t mongrel2_reply_handler(backend_t *backend, request_t *request){ // {{{
	return -ENOSYS;
} // }}}

static backend_t mongrel2_reply_proto = {
	.class          = "modules/c_mongrel2_reply",
	.supported_api  = API_HASH,
	.backend_type_hash = {
		.func_handler = &mongrel2_reply_handler
	},
};

static ssize_t mongrel2_parse_handler(backend_t *backend, fastcall_header *hargs){ // {{{
	ssize_t                ret;
	char                  *p, *uuid_p, *connid_p, *path_p, *header_p, *body_p, *e;
	uintmax_t                  uuid_l,  connid_l,  path_l,  header_l,  body_l;
	fastcall_write        *wargs             = (fastcall_write *)hargs;
	
	if(hargs->action != ACTION_WRITE)
		return -ENOSYS;
	
	p = wargs->buffer;
	e = wargs->buffer + wargs->buffer_size;
	
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
	
	request_t request[] = {
		{ HK(uuid),     DATA_RAW(uuid_p,   uuid_l)   },
		{ HK(clientid), DATA_RAW(connid_p, connid_l) },
		{ HK(path),     DATA_RAW(path_p,   path_l)   },
		{ HK(headers),  DATA_RAW(header_p, header_l) },
		{ HK(body),     DATA_RAW(body_p,   body_l)   },
		hash_end
	};
	
	return ( (ret = (backend_pass(backend, request)) < 0) ) ? ret : -EEXIST;
} // }}}

static backend_t mongrel2_parse_proto = {
	.class          = "modules/c_mongrel2_parse",
	.supported_api  = API_FAST,
	.backend_type_fast = {
		.func_handler = (f_fast_func)&mongrel2_parse_handler
	},
};

int main(void){
	class_register(&mongrel2_reply_proto);
	class_register(&mongrel2_parse_proto);
	return 0;
}
