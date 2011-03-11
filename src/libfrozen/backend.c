#include <libfrozen.h>

// TODO remake init()

static list   chains;
static list   backends;

/* chains - private {{{1 */
static void __attribute__ ((constructor)) __chain_init(){
	list_init(&chains);
	list_init(&backends);
}

static int chain_find_by_name(void *p_chain, void *p_name, void *p_chain_t){
	if(strcmp(((chain_t *)p_chain)->name, p_name) == 0){
		*(chain_t **)p_chain_t = (chain_t *)p_chain;
		return ITER_BREAK;
	}
	return ITER_CONTINUE;
}
/* }}}1 */
/* chains - public {{{1 */
void      _chain_register(chain_t *chain){
	list_push(&chains, chain);
}

chain_t * chain_new(char *name){
	int      ret;
	chain_t *chain;
	chain_t *chain_proto = NULL;
	
	list_iter(&chains, (iter_callback)&chain_find_by_name, name, &chain_proto);
	if(chain_proto == NULL)
		return NULL;
	
	chain = (chain_t *)malloc(sizeof(chain_t));
	if(chain == NULL)
		return NULL;
	
	memcpy(chain, chain_proto, sizeof(chain_t));
	
	chain->cnext = NULL;
	
	ret = chain->func_init(chain);
	if(ret != 0){
		free(chain);
		return NULL;
	}
	
	return chain;
}
void chain_destroy(chain_t *chain){
	if(chain == NULL)
		return;

	chain->func_destroy(chain);
	
	free(chain);
}
inline int  chain_configure    (chain_t *chain, hash_t *config){ return chain->func_configure(chain, config); }

ssize_t     chain_query        (chain_t *chain, request_t *request){
	f_crwd        func       = NULL;
	ssize_t       ret;
	uint32_t      r_action;
	
	if(chain == NULL || request == NULL)
		return_error(-EINVAL, "chain_query 'chain' or 'request' is null\n");
	
	hash_data_copy(ret, TYPE_INT32, r_action, request, HK(action));
	if(ret != 0)
		return_error(-EINVAL, "chain_query request 'action' not set\n");
	
	switch(r_action){
		case ACTION_CRWD_CREATE: func = chain->chain_type_crwd.func_create; break;
		case ACTION_CRWD_READ:   func = chain->chain_type_crwd.func_get   ; break;
		case ACTION_CRWD_WRITE:  func = chain->chain_type_crwd.func_set   ; break;
		case ACTION_CRWD_DELETE: func = chain->chain_type_crwd.func_delete; break;
		case ACTION_CRWD_MOVE:   func = chain->chain_type_crwd.func_move  ; break;
		case ACTION_CRWD_COUNT:  func = chain->chain_type_crwd.func_count ; break;
		case ACTION_CRWD_CUSTOM: func = chain->chain_type_crwd.func_custom; break;
		default:
			return -EINVAL;
	};	
	
	if(func == NULL)
		return chain_query(chain->cnext, request);
	
	return func(chain, request);
}

ssize_t     chain_next_query   (chain_t *chain, request_t *request){
	return chain_query(chain->cnext, request);
}

/* }}}1 */

/* backends - private {{{1 */
static ssize_t backend_iter_chain_init(hash_t *config, void *p_backend, void *p_chain){
	ssize_t    ret;
	chain_t   *chain;
	char      *chain_name;
	hash_t    *chain_config;
	
	if(data_value_type(hash_item_data(config)) != TYPE_HASHT)
		return ITER_BREAK;
	
	chain_config = (hash_t *) data_value_ptr(hash_item_data(config));
	
	hash_data_copy(ret, TYPE_STRING, chain_name, chain_config, HK(name));
	if(ret != 0) return ITER_BREAK;
	
	if( (chain = chain_new(chain_name)) == NULL)
		return ITER_BREAK;
	
	chain->cnext = *(chain_t **)p_chain;
	
	if(chain_configure(chain, chain_config) != 0)
		goto cleanup;
	
	*(chain_t **)p_chain = chain;
	
	return ITER_CONTINUE;

cleanup:	
	chain_destroy(chain);
	return ITER_BREAK;
}

static ssize_t backend_iter_find(void *p_backend, void *p_name, void *p_backend_t){
	backend_t *backend = (backend_t *)p_backend;
	
	if(strcmp(backend->name, p_name) == 0){
		*(backend_t **)p_backend_t = backend;
		return ITER_BREAK;
	}
	return ITER_CONTINUE;
}

static ssize_t backend_iter_backend_new(hash_t *config, void *null1, void *null2){
	if(data_value_type(hash_item_data(config)) != TYPE_HASHT)
		return ITER_CONTINUE;
	
	if(backend_new( data_value_ptr(hash_item_data(config)) ) == NULL)
		return ITER_BREAK;
	
	return ITER_CONTINUE;
}

/* }}}1 */
/* backends - public {{{1 */

backend_t *  backend_new      (hash_t *config){
	ssize_t     ret;
	DT_STRING   name = NULL;
	DT_HASHT    chains;
	backend_t  *backend = NULL;
	
	hash_data_copy(ret, TYPE_HASHT,  chains, config, HK(chains)); if(ret != 0) return NULL;
	hash_data_copy(ret, TYPE_STRING, name,   config, HK(name));
	
	// register new one
	if( (backend = malloc(sizeof(backend_t))) == NULL)
		return NULL;
	
	backend->chain = NULL;
	backend->refs  = 1;
	backend->name  = name ? strdup(name) : NULL;
	
	ret = hash_iter(chains, &backend_iter_chain_init, backend, &backend->chain);
	if(ret == ITER_BREAK){
		backend_destroy(backend);
		return NULL;
	}
	
	list_push(&backends, backend);
	
	return backend;
}

backend_t *  backend_find(char *name){
	backend_t  *backend = NULL;
	
	list_iter(&backends, (iter_callback)&backend_iter_find, name, &backend);
	return backend;
}

backend_t *  backend_acquire(char *name){
	backend_t  *backend = backend_find(name);
	if(backend != NULL)
		backend->refs++;
	
	return backend;
}

ssize_t      backend_bulk_new(hash_t *config){
	if(hash_iter(config, &backend_iter_backend_new, NULL, NULL) == ITER_BREAK){
		// TODO destory previosly allocated backends
		return -EINVAL;
	}
	return 0;
}

char *          backend_get_name        (backend_t *backend){
	return backend->name;
}

ssize_t      backend_query        (backend_t *backend, request_t *request){
	if(backend == NULL || request == NULL)
		return -EINVAL;
	
	return chain_query(backend->chain, request);
}

void         backend_destroy  (backend_t *backend){
	if(backend == NULL)
		return;
	
	if(--backend->refs > 0)
		return;
	
	list_unlink(&backends, backend); 
	
	chain_t *chain_curr = backend->chain;
	chain_t *chain_next;
	
	while(chain_curr != NULL){
		chain_next = chain_curr->cnext;
		chain_destroy(chain_curr);
		chain_curr = chain_next;
	}
	
	if(backend->name)
		free(backend->name);
	free(backend);
}
/* }}}1 */

request_actions request_str_to_action(char *string){
	if(strcasecmp(string, "create") == 0) return ACTION_CRWD_CREATE;
	if(strcasecmp(string, "write")  == 0) return ACTION_CRWD_WRITE;
	if(strcasecmp(string, "read")   == 0) return ACTION_CRWD_READ;
	if(strcasecmp(string, "delete") == 0) return ACTION_CRWD_DELETE;
	if(strcasecmp(string, "move")   == 0) return ACTION_CRWD_MOVE;
	if(strcasecmp(string, "count")  == 0) return ACTION_CRWD_COUNT;
	if(strcasecmp(string, "custom") == 0) return ACTION_CRWD_CUSTOM;
	return REQUEST_INVALID;
}

