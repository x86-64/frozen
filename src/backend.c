#include <libfrozen.h>
#include <backend.h>

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
	
	chain->next = NULL;
	
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

ssize_t     chain_query        (chain_t *chain, request_t *request, buffer_t *buffer){
	unsigned int  action;
	f_crwd        func       = NULL;
	
	if(chain == NULL || request == NULL)
		return_error(-EINVAL, "chain_query 'chain' or 'request' is null\n");
	
	if(hash_get_copy(request, "action", TYPE_INT32, &action, sizeof(action)) != 0)
		return_error(-EINVAL, "chain_query request 'action' not set\n");
	
	switch(action){
		case ACTION_CRWD_CREATE: func = chain->chain_type_crwd.func_create; break;
		case ACTION_CRWD_READ:   func = chain->chain_type_crwd.func_get   ; break;
		case ACTION_CRWD_WRITE:  func = chain->chain_type_crwd.func_set   ; break;
		case ACTION_CRWD_DELETE: func = chain->chain_type_crwd.func_delete; break;
		case ACTION_CRWD_MOVE:   func = chain->chain_type_crwd.func_move  ; break;
		case ACTION_CRWD_COUNT:  func = chain->chain_type_crwd.func_count ; break;
		default:
			return -EINVAL;
	};	
	
	if(func == NULL)
		return chain_query(chain->next, request, buffer);
	
	return func(chain, request, buffer);
}

ssize_t     chain_next_query   (chain_t *chain, request_t *request, buffer_t *buffer){
	return chain_query(chain->next, request, buffer);
}

/* }}}1 */

/* backends - private {{{1 */
static int backend_iter_chain_init(void *p_setting, void *p_backend, void *p_chain){
	int        ret;
	chain_t   *chain;
	char      *chain_name;
	setting_t *setting = (setting_t *)p_setting;
	
	chain_name = setting_get_child_string(setting, "name");
	if(chain_name == NULL)
		return ITER_BREAK;
	
	chain = chain_new(chain_name);
	if(chain == NULL)
		return ITER_BREAK;
	
	chain->next = *(chain_t **)p_chain;
	
	//printf("initing: %s\n", chain_name);
	ret = chain_configure(chain, setting);
	if(ret != 0)
		goto cleanup;
	
	*(chain_t **)p_chain = chain;
	
	return ITER_CONTINUE;

cleanup:	
	chain_destroy(chain);
	return ITER_BREAK;
}

static int backend_iter_find(void *p_backend, void *p_name, void *p_backend_t){
	backend_t *backend = (backend_t *)p_backend;
	if(strcmp(backend->name, p_name) == 0){
		*(backend_t **)p_backend_t = backend;
		return ITER_BREAK;
	}
	return ITER_CONTINUE;
}

/* }}}1 */
/* backends - public {{{1 */

backend_t *  backend_new      (char *name, setting_t *setting){
	int ret;
	
	if(name == NULL || setting == NULL)
		return NULL;	
	
	backend_t *backend = (backend_t *)malloc(sizeof(backend_t));
	if(backend == NULL)
		return NULL;
	
	backend->name  = strdup(name);
	backend->chain = NULL;
	
	ret = setting_iter_child(setting, (iter_callback)&backend_iter_chain_init, backend, &backend->chain);
	if(ret == ITER_BROKEN){
		backend_destory(backend);
		return NULL;
	}
	
	list_push(&backends, backend);
	
	return backend;
}

backend_t *  backend_find_by_name(char *name){
	backend_t *backend = NULL;
	
	list_iter(&backends, (iter_callback)&backend_iter_find, name, &backend);
	
	return backend;
}

ssize_t      backend_query        (backend_t *backend, request_t *request, buffer_t *buffer){
	if(backend == NULL || request == NULL)
		return -EINVAL;
	
	return chain_query(backend->chain, request, buffer);
}

void         backend_destory  (backend_t *backend){
	if(backend == NULL)
		return;
	
	list_unlink(&backends, backend); 
	
	chain_t *chain_curr = backend->chain;
	chain_t *chain_next;
	
	while(chain_curr != NULL){
		chain_next = chain_curr->next;
		chain_destroy(chain_curr);
		chain_curr = chain_next;
	}
	
	free(backend->name);
	free(backend);
}
/* }}}1 */

