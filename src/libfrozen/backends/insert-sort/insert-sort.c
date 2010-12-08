#line 1 "backends/insert-sort.c.m4"
#include <libfrozen.h>
#include <backend.h>

#define KEY_FOUND     0
#define KEY_NOT_FOUND 1

typedef struct sort_proto_t     sort_proto_t;
typedef struct sorts_user_data  sorts_user_data;

typedef ssize_t (*func_find_key)(sorts_user_data *data, data_t *, data_ctx_t *, data_t *, data_ctx_t *);

struct sort_proto_t {
	char           *name;
	func_find_key   func_find;
};

struct sorts_user_data {
	chain_t         *chain;
	sort_proto_t    *engine;
};

/* m4 {{{
#line 25

}}} */
#line 1 "backends/insert-sort/binsearch.c"

ssize_t             sorts_binsearch_find (sorts_user_data *data, data_t *buffer1, data_ctx_t *buffer1_ctx, data_t *key_out, data_ctx_t *key_out_ctx){
	ssize_t         ret;
	off_t           range_start, range_end, current;
	data_t          d_current = DATA_PTR_OFFT(&current);
	
	range_start = range_end = 0;
	
	hash_t  req_count[] = {
		{ "action", DATA_INT32(ACTION_CRWD_COUNT) },
		{ "buffer", DATA_PTR_OFFT(&range_end)     },
		hash_end
	};
	ret = chain_next_query(data->chain, req_count);
	if(ret <= 0 || range_start == range_end){
		current = range_end;
		
		ret = KEY_NOT_FOUND;
		goto exit;
	}
	
	data_t  backend = DATA_CHAINT(data->chain);
	hash_t  backend_ctx[] = {
		{ "offset",  DATA_PTR_OFFT(&current)  },
		hash_end
	};
	
	/* check last element of list to fast-up incremental inserts */
	current = range_end - 1;
	if( (ret = data_cmp(buffer1, buffer1_ctx, &backend, backend_ctx)) >= 0){
		current = range_end;
		ret = (ret == 0) ? KEY_FOUND : KEY_NOT_FOUND;
		
		goto exit;
	}
	
	while(range_start + 1 < range_end){
		current = range_start + ((range_end - range_start) / 2);
		
		ret = data_cmp(buffer1, buffer1_ctx, &backend, backend_ctx);
		
		/*
		printf("range: %x-%x curr: %x ret: %x\n",
			(unsigned int)range_start, (unsigned int)range_end,
			(unsigned int)current, (unsigned int)ret
		);
		*/
		
		if(ret == 0){
			ret = KEY_FOUND;
			goto exit;
		}else if(ret < 0){
			range_end   = current;
		}else{
			range_start = current;
		}
	}
	current = range_start;
	if(data_cmp(buffer1, buffer1_ctx, &backend, backend_ctx) <= 0)
		current = range_start;
	else
		current = range_end;
	
	ret = KEY_NOT_FOUND;
exit:
	data_transfer(
		key_out,    key_out_ctx,
		&d_current, NULL
	);
	return ret;
}


#line 73
        
#line 27 "backends/insert-sort.c.m4"


/* sort protos {{{ */
sort_proto_t  sort_protos[] = { { .name = "binsearch", .func_find = &sorts_binsearch_find },
#line 30
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
	
	free(data);
	chain->user_data = NULL;
	
	return 0;
} // }}}
static int sorts_configure(chain_t *chain, hash_t *config){ // {{{
	int              i;
	char            *sort_engine_str;
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	data->chain = chain;
	
	/* get engine info */
	if(hash_get_typed(config, TYPE_STRING, "engine", (void **)&sort_engine_str, NULL) != 0)
		return_error(-EINVAL, "chain 'insert-sort' parameter 'engine' not supplied\n");
	
	for(i=0, data->engine = NULL; i<sort_protos_size; i++){
		if(strcasecmp(sort_protos[i].name, sort_engine_str) == 0){
			data->engine = &(sort_protos[i]);
			break;
		}
	}
	if(data->engine == NULL)
		return_error(-EINVAL, "chain 'insert-sort' engine not found\n");
	
	return 0;
} // }}}
/* }}} */
/* crwd handlers {{{ */
static ssize_t sorts_create(chain_t *chain, request_t *request){
	return -1;
}

static ssize_t sorts_set   (chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *buffer, *key_out;
	data_ctx_t      *buffer_ctx, *key_out_ctx;
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	if( (buffer = hash_get_data(request, "buffer")) == NULL)
		return -EINVAL;
	buffer_ctx = hash_get_data_ctx(request, "buffer");
	
	if( (key_out = hash_get_data(request, "key_out")) == NULL)
		return -EINVAL;
	key_out_ctx = hash_get_data_ctx(request, "key_out");
	
	// TODO underlying locking and threading
	// next("lock");
	
	ret = data->engine->func_find(data, buffer, buffer_ctx, key_out, key_out_ctx);
	/*
	if(ret == KEY_FOUND){
		
	}else{
		
	}
	*/
	request_t req_insert[] = {
		{ "key",    *key_out                    },
		{ "insert", DATA_INT32(1)               },
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
	sorts_user_data *data = (sorts_user_data *)chain->user_data;
	
	if( (r_funcname = hash_find_typed(request, TYPE_STRING, "function")) == NULL)
		return -EINVAL;
	
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
