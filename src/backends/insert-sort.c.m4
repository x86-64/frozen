#include <libfrozen.h>
#include <backend.h>

#define KEY_FOUND     0
#define KEY_NOT_FOUND 1

typedef struct sort_proto_t     sort_proto_t;
typedef struct sorts_user_data  sorts_user_data;

typedef ssize_t (*func_find_key)(sorts_user_data *data, buffer_t *buffer1, buffer_t *key_oid);
// TODO move usage key_oid to data struct

struct sort_proto_t {
	char           *name;
	func_find_key   func_find;
};

struct sorts_user_data {
	chain_t         *chain;
	sort_proto_t    *engine;
	data_ctx_t       cmp_ctx;
	
	off_t            key;
	buffer_t         key_buffer;
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
	sorts_user_data *user_data = calloc(1, sizeof(sorts_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	
	return 0;
} // }}}
static int sorts_destroy(chain_t *chain){ // {{{
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	// TODO destroy without configure cause crash

	data_ctx_destroy(&data->cmp_ctx);
	
	buffer_destroy(&data->key_buffer);
	
	free(chain->user_data);
	
	chain->user_data = NULL;
	
	return 0;
} // }}}
static int sorts_configure(chain_t *chain, hash_t *config){ // {{{
	char       *sort_engine_str;
	char       *info_type_str;
	hash_t     *r_context;
	data_type   info_type;
	int         i;
	
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	data->chain = chain;
	
	/* get engine info */
	if(hash_get_typed(config, TYPE_STRING, "sort-engine", (void **)&sort_engine_str, NULL) != 0)
		return_error(-EINVAL, "chain 'insert-sort' parameter 'sort-engine' not supplied\n");
	
	for(i=0, data->engine = NULL; i<sort_protos_size; i++){
		if(strcasecmp(sort_protos[i].name, sort_engine_str) == 0){
			data->engine = &(sort_protos[i]);
			break;
		}
	}
	if(data->engine == NULL)
		return_error(-EINVAL, "chain 'insert-sort' engine not found\n");
	
	/* get type of data */
	if(hash_get_typed(config, TYPE_STRING, "type", (void **)&info_type_str, NULL) != 0)
		return_error(-EINVAL, "chain 'insert-sort' type not defined\n");
	if( (info_type = data_type_from_string(info_type_str)) == -1)
		return_error(-EINVAL, "chain 'insert-sort' type invalid\n");
	
	/* get context */
	r_context = hash_find(config, "type-context");
	if( data_ctx_init(&data->cmp_ctx, info_type, r_context) != 0)
		return_error(-EINVAL, "chain 'insert-sort' data ctx create failed\n");
	
	buffer_init_from_bare(&data->key_buffer, &data->key, sizeof(data->key));
	// TODO fix mem leak
	
	return 0;
} // }}}
/* }}} */
/* crwd handlers {{{ */
static ssize_t sorts_create(chain_t *chain, request_t *request){
	return -1;
}

static ssize_t sorts_set   (chain_t *chain, request_t *request){
	ssize_t          ret;
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	buffer_t        *buffer, *keyout_buffer;
	void            *key_ptr;
	
	if( hash_get_typed(request, TYPE_BUFFERT, "buffer", (void **)&buffer, NULL) != 0)
		return -EINVAL;
	if( hash_get_typed(request, TYPE_BUFFERT, "key_buffer", (void **)&keyout_buffer, NULL) != 0)
		keyout_buffer = &data->key_buffer;
	
	// TODO TOP think about conversion and in/out keys. key_buffer can be just key pointing to buffer and expanding
	// where it need. User also can supply non-buffer value and reject any outgoing keys
	
	// TODO underlying locking and threading
	// next("lock");
	
	ret = data->engine->func_find(data, buffer, keyout_buffer);
	/*
	if(ret == KEY_FOUND){
		
	}else{
		
	}
	*/
	key_ptr = buffer_defragment(keyout_buffer);
	hash_t set_request[] = {
		{ "key",    DATA_PTR_OFFT(key_ptr)      },
		{ "insert", DATA_INT32(1)               },
		hash_next(request)
	};
	ret = chain_next_query(chain, set_request);
	if(ret <= 0)
		return ret;
	
	if(keyout_buffer == &data->key_buffer){
		ret = buffer_get_size(keyout_buffer);
		buffer_memcpy(buffer, 0, keyout_buffer, 0, ret);
	}
	// next("unlock");
	return ret;
}

static ssize_t sorts_custom(chain_t *chain, request_t *request){
	hash_t *r_funcname;
	
	//sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	if( (r_funcname = hash_find_typed(request, TYPE_STRING, "function")) == NULL)
		return -EINVAL;
	
	/*
	if(strcmp((char *)r_funcname->value, "search") == 0){
		return data->engine->func_find(data, buffer, &data->key_buffer); // do search
	}
	*/
	
	return -EINVAL;
}
/* }}} */

static chain_t chain_insert_sort = {
	"insert-sort",
	CHAIN_TYPE_CRWD,
	&sorts_init,
	&sorts_configure,
	&sorts_destroy,
	{{
		.func_create = &sorts_create,
		.func_set    = &sorts_set,
		.func_custom = &sorts_custom
	}}
};
CHAIN_REGISTER(chain_insert_sort)

/* vim: set filetype=c: */
