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
	crwd_fastcall    fc_table;
	data_ctx_t       cmp_ctx;
	
	hash_t          *set_request;
	
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
	
	// TODO destory without configure cause crash

	fc_crwd_destory(&data->fc_table);
	data_ctx_destory(&data->cmp_ctx);
	hash_free(data->set_request);
	
	buffer_destroy(&data->key_buffer);
	
	free(chain->user_data);
	
	chain->user_data = NULL;
	
	return 0;
} // }}}
static int sorts_configure(chain_t *chain, setting_t *config){ // {{{
	char       *sort_engine_str;
	char       *info_type_str;
	char       *cmp_context_str;
	data_type   info_type;
	int         i;
	
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	data->chain = chain;
	
	/* get engine info */
	if( (sort_engine_str = setting_get_child_string(config, "sort-engine")) == NULL)
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
	if( (info_type_str = setting_get_child_string(config, "type")) == NULL)
		return_error(-EINVAL, "chain 'insert-sort' type not defined\n");
	if( (info_type = data_type_from_string(info_type_str)) == -1)
		return_error(-EINVAL, "chain 'insert-sort' type invalid\n");
	
	/* get context */
	cmp_context_str = setting_get_child_string(config, "type-context");
	if( data_ctx_init(&data->cmp_ctx, info_type, cmp_context_str) != 0)
		return_error(-EINVAL, "chain 'insert-sort' data ctx create failed\n");
	
	if( fc_crwd_init(&data->fc_table) != 0)
		return_error(-EINVAL, "chain 'insert-sort' fastcall table init failed\n");
	
	buffer_init_from_bare(&data->key_buffer, &data->key, sizeof(data->key));
	// TODO fix mem leak
	
	data->set_request  = hash_new();
	
	return 0;
} // }}}
/* }}} */
/* crwd handlers {{{ */
static ssize_t sorts_create(chain_t *chain, request_t *request, buffer_t *buffer){
	return -1;
}

static ssize_t sorts_set   (chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t       ret;
	unsigned int  insert = 1;
	
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	// TODO underlying locking and threading
	// next("lock");
	
	ret = data->engine->func_find(data, buffer, &data->key_buffer);
	/*
	if(ret == KEY_FOUND){
		
	}else{
		
	}
	*/
	hash_empty        (data->set_request);
	hash_assign_layer (data->set_request, request);
	hash_set          (data->set_request, "key",    TYPE_INT64, &data->key);
	hash_set          (data->set_request, "insert", TYPE_INT32, &insert);
	
	// next("unlock");
	return chain_next_query(chain, data->set_request, buffer);
}

static ssize_t sorts_custom(chain_t *chain, request_t *request, buffer_t *buffer){
	data_t *funcname;
	
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	if(hash_get(request, "function", TYPE_STRING, &funcname, NULL) != 0)
		return -EINVAL;
	
	if(strcmp((char *)funcname, "search") == 0){
		return data->engine->func_find(data, buffer, &data->key_buffer); // do search
	}
	
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
