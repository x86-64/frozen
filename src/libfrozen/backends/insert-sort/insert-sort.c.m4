#include <libfrozen.h>
#include <backend.h>

#define KEY_FOUND     0
#define KEY_NOT_FOUND 1

typedef struct sort_proto_t     sort_proto_t;
typedef struct sorts_userdata  sorts_userdata;

typedef ssize_t (*func_find_key)(sorts_userdata *data, data_t *, data_ctx_t *, data_t *, data_ctx_t *);

struct sort_proto_t {
	char           *name;
	func_find_key   func_find;
};

struct sorts_userdata {
	chain_t         *chain;
	sort_proto_t    *engine;
	hash_key_t       sort_field;
};

/* m4 {{{
m4_define(`REGISTER', `
        m4_define(`VAR_ARRAY', m4_defn(`VAR_ARRAY')`$1,
')')
}}} */
m4_include(backends/insert-sort/binsearch.c)

/* sort protos {{{ */
sort_proto_t  sort_protos[] = { VAR_ARRAY() };
#define       sort_protos_size sizeof(sort_protos) / sizeof(sort_proto_t)
// }}}


/* init {{{ */
static int sorts_init(chain_t *chain){ // {{{
	sorts_userdata *userdata = calloc(1, sizeof(sorts_userdata));
	if(userdata == NULL)
		return -ENOMEM;
	
	chain->userdata = userdata;
	
	return 0;
} // }}}
static int sorts_destroy(chain_t *chain){ // {{{
	sorts_userdata *data = (sorts_userdata *)chain->userdata;
	
	free(data);
	chain->userdata = NULL;
	
	return 0;
} // }}}
static int sorts_configure(chain_t *chain, hash_t *config){ // {{{
	ssize_t          ret;
	unsigned int     i;
	char            *sort_engine_str;
	char            *sort_field_str = "buffer";
	sorts_userdata  *data = (sorts_userdata *)chain->userdata;
	
	data->chain = chain;
	
	/* get engine info */
	hash_data_copy(ret, TYPE_STRING, sort_field_str,  config, HK(field));
	hash_data_copy(ret, TYPE_STRING, sort_engine_str, config, HK(engine));
	if(ret != 0)
		return_error(-EINVAL, "chain 'insert-sort' parameter 'engine' not supplied\n");
	
	for(i=0, data->engine = NULL; i<sort_protos_size; i++){
		if(strcasecmp(sort_protos[i].name, sort_engine_str) == 0){
			data->engine = &(sort_protos[i]);
			break;
		}
	}
	if(data->engine == NULL)
		return_error(-EINVAL, "chain 'insert-sort' engine not found\n");
	
	data->sort_field = hash_string_to_key(sort_field_str);
	
	return 0;
} // }}}
/* }}} */
/* crwd handlers {{{ */
static ssize_t sorts_set   (chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *buffer,     *key_out;
	data_ctx_t      *buffer_ctx, *key_out_ctx;
	sorts_userdata  *data = (sorts_userdata *)chain->userdata;
	
	hash_data_find(request, data->sort_field, &buffer,  &buffer_ctx);
	hash_data_find(request, HK(offset_out),   &key_out, &key_out_ctx);
	if(buffer == NULL || key_out == NULL)
		return -EINVAL;
	
	// TODO underlying locking and threading
	// next("lock");
	
	ret = data->engine->func_find(data, buffer, buffer_ctx, key_out, key_out_ctx);
	/*
	if(ret == KEY_FOUND){
		
	}else{
		
	}
	*/
	request_t req_insert[] = {
		{ HK(offset), *key_out                    },
		{ HK(insert), DATA_INT32(1)               },
		hash_next(request)
	};
	ret = chain_next_query(chain, req_insert);
	if(ret <= 0)
		return ret;
	
	/*
	if(keyout_buffer == &data->key_buffer){
		ret = buffer_get_size(keyout_buffer);
		buffer_memcpy(buffer, 0, keyout_buffer, 0, ret);
	}
	*/
	// next("unlock");
	return ret;
}

static ssize_t sorts_custom(chain_t *chain, request_t *request){
	/*
	hash_t *r_funcname;
	sorts_userdata *data = (sorts_userdata *)chain->userdata;
	
	if( (r_funcname = hash_find_typed(request, TYPE_STRING, HK(function))) == NULL)
		return -EINVAL;
	
	if(strcmp((char *)r_funcname->value, "search") == 0){
		return data->engine->func_find(data, buffer, &data->key_buffer); // do search
	}
	*/
	(void)chain; (void)request;
	return -EINVAL;
}
/* }}} */

static chain_t chain_insert_sort = {
	"insert-sort",
	CHAIN_TYPE_CRWD,
	.func_init      = &sorts_init,
	.func_configure = &sorts_configure,
	.func_destroy   = &sorts_destroy,
	{{
		.func_set    = &sorts_set,
		.func_custom = &sorts_custom
	}}
};
CHAIN_REGISTER(chain_insert_sort)

/* vim: set filetype=c: */
