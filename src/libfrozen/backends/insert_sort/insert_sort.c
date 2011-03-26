#line 1 "insert_sort.m4"
#include <libfrozen.h>
#include <backend.h>

#define EMODULE 13
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
	backend_t         *backend;
	sort_proto_t    *engine;
	hash_key_t       sort_field;
};

/* m4 {{{
#line 27

}}} */
#line 1 "binsearch.c"

ssize_t             sorts_binsearch_find (sorts_userdata *data, data_t *buffer1, data_ctx_t *buffer1_ctx, data_t *key_out, data_ctx_t *key_out_ctx){
	ssize_t         ret;
	off_t           range_start, range_end, current;
	data_t          d_current = DATA_PTR_OFFT(&current);
	
	range_start = range_end = 0;
	
	hash_t  req_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT) },
		{ HK(buffer), DATA_PTR_OFFT(&range_end)     },
		hash_end
	};
	ret = backend_pass(data->backend, req_count);
	if(ret <= 0 || range_start == range_end){
		current = range_end;
		
		ret = KEY_NOT_FOUND;
		goto exit;
	}
	
	data_t  backend = DATA_BACKENDT(data->backend);
	hash_t  backend_ctx[] = {
		{ HK(offset),  DATA_PTR_OFFT(&current)  },
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
        
#line 29 "insert_sort.m4"


/* sort protos {{{ */
sort_proto_t  sort_protos[] = { { .name = "binsearch", .func_find = &sorts_binsearch_find },
#line 32
 };
#define       sort_protos_size sizeof(sort_protos) / sizeof(sort_proto_t)
// }}}


/* init {{{ */
static int sorts_init(backend_t *backend){ // {{{
	sorts_userdata *userdata = calloc(1, sizeof(sorts_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	backend->userdata = userdata;
	
	return 0;
} // }}}
static int sorts_destroy(backend_t *backend){ // {{{
	sorts_userdata *data = (sorts_userdata *)backend->userdata;
	
	free(data);
	backend->userdata = NULL;
	
	return 0;
} // }}}
static int sorts_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t          ret;
	unsigned int     i;
	char            *sort_engine_str;
	char            *sort_field_str = "buffer";
	sorts_userdata  *data = (sorts_userdata *)backend->userdata;
	
	data->backend = backend;
	
	/* get engine info */
	hash_data_copy(ret, TYPE_STRING, sort_field_str,  config, HK(field));
	hash_data_copy(ret, TYPE_STRING, sort_engine_str, config, HK(engine));
	if(ret != 0)
		return error("backend insert-sort parameter engine not supplied");
	
	for(i=0, data->engine = NULL; i<sort_protos_size; i++){
		if(strcasecmp(sort_protos[i].name, sort_engine_str) == 0){
			data->engine = &(sort_protos[i]);
			break;
		}
	}
	if(data->engine == NULL)
		return error("backend insert-sort engine not found");
	
	data->sort_field = hash_string_to_key(sort_field_str);
	
	return 0;
} // }}}
/* }}} */
/* crwd handlers {{{ */
static ssize_t sorts_set   (backend_t *backend, request_t *request){
	ssize_t          ret;
	data_t          *buffer,     *key_out;
	data_ctx_t      *buffer_ctx, *key_out_ctx;
	sorts_userdata  *data = (sorts_userdata *)backend->userdata;
	
	hash_data_find(request, data->sort_field, &buffer,  &buffer_ctx);  if(buffer == NULL)  return warning("no buffer supplied");
	hash_data_find(request, HK(offset_out),   &key_out, &key_out_ctx); if(key_out == NULL) return warning("no key_out supplied");
	
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
		{ HK(insert), DATA_UINT32T(1)               },
		hash_next(request)
	};
	ret = backend_pass(backend, req_insert);
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

static ssize_t sorts_custom(backend_t *backend, request_t *request){
	/*
	hash_t *r_funcname;
	sorts_userdata *data = (sorts_userdata *)backend->userdata;
	
	if( (r_funcname = hash_find_typed(request, TYPE_STRING, HK(function))) == NULL)
		return -EINVAL;
	
	if(strcmp((char *)r_funcname->value, "search") == 0){
		return data->engine->func_find(data, buffer, &data->key_buffer); // do search
	}
	*/
	(void)backend; (void)request;
	return -ENOSYS;
}
/* }}} */

backend_t backend_insert_sort = {
	"insert-sort",
	.supported_api = API_CRWD,
	.func_init      = &sorts_init,
	.func_configure = &sorts_configure,
	.func_destroy   = &sorts_destroy,
	{{
		.func_set    = &sorts_set,
		.func_custom = &sorts_custom
	}}
};

/* vim: set filetype=c: */
