#line 1 "backends/insert-sort.c.m4"
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
	buffer_t        *key_out;
	
};

/* m4 {{{
#line 30

}}} */
#line 1 "backends/insert-sort/binsearch.c"

ssize_t             sorts_binsearch_find (sorts_user_data *data, buffer_t *buffer1, buffer_t *key_out){
	ssize_t         ret;
	off_t           range_start, range_end, current;
	size_t          range_elements;
	buffer_t       *buffer2 = NULL;
	
	range_start = 0;
	if( fc_crwd_chain(&data->fc_table, ACTION_CRWD_COUNT, &range_start, sizeof(range_start)) <= 0)
		return -EINVAL;
	
	if(range_start == range_end){
		buffer_write(key_out, 0, &range_end, sizeof(range_end));
		return KEY_NOT_FOUND;
	}
	
	while(1){
		range_elements = (range_end - range_start);
		current        = range_start + (range_elements / 2);
		
		ret = data_buffer_cmp(&data->cmp_ctx, buffer1, 0, buffer2, 0);
		if(ret == 0){
			buffer_write(key_out, 0, &current, sizeof(current));
			return KEY_FOUND;
		}else if(ret < 0){
			range_end   = current;
		}else{
			range_start = current;
		}
		
		if(range_elements <= 1){
			buffer_write(key_out, 0, &current, sizeof(current));
			return KEY_NOT_FOUND;
		}
	}
}


#line 38
        
#line 32 "backends/insert-sort.c.m4"


/* sort protos {{{ */
sort_proto_t  sort_protos[] = { { .name = "binsearch", .func_find = &sorts_binsearch_find },
#line 35
 };
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
		return_error(-EINVAL, "chain 'insert-sort' parameter 'sort-engine' not supplied");
	
	for(i=0, data->engine = NULL; i<sort_protos_size; i++){
		if(strcasecmp(sort_protos[i].name, sort_engine_str) == 0){
			data->engine = &(sort_protos[i]);
			break;
		}
	}
	if(data->engine == NULL)
		return -EINVAL;
	
	/* get type of data */
	if( (info_type_str = setting_get_child_string(config, "type")) == NULL)
		return -EINVAL;
	if( (info_type = data_type_from_string(info_type_str)) == -1)
		return -EINVAL;
	
	/* get context */
	cmp_context_str = setting_get_child_string(config, "type-context");
	if( data_ctx_init(&data->cmp_ctx, info_type, cmp_context_str) != 0)
		return -EINVAL;
	
	
	if( fc_crwd_init(&data->fc_table) != 0)
		return_error(-EINVAL, "chain 'insert-sort' fastcall table init failed\n");
	
	if( (data->key_out = buffer_alloc()) == NULL)
		return -EINVAL;
	// TODO fix mem leak
	
	return 0;
} // }}}
/* }}} */
/* crwd handlers {{{ */
static ssize_t sorts_create(chain_t *chain, request_t *request, buffer_t *buffer){
	return -1;
}

static ssize_t sorts_set   (chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t  ret;
	
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	// TODO underlying locking and threading
	// next("lock");
	
	ret = data->engine->func_find(data, buffer, data->key_out);
	printf("find: %x\n", (unsigned int)ret);
	
	// cursor = search();
	// cursor_insert();
	// free(cursor)
	
	// next("unlock");
	//return chain_next_query(chain, request, buffer);
	return -1;
}

static ssize_t sorts_custom(chain_t *chain, request_t *request, buffer_t *buffer){
	data_t *funcname;
	
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	if(hash_get(request, "function", TYPE_STRING, &funcname, NULL) != 0)
		return -EINVAL;
	
	if(strcmp((char *)funcname, "search") == 0){
		return data->engine->func_find(data, buffer, data->key_out); // do search
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
