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
inline int  chain_configure    (chain_t *chain, setting_t *setting){ return chain->func_configure(chain, setting); }

ssize_t     chain_query        (chain_t *chain, request_t *request, buffer_t *buffer){
	f_crwd        func       = NULL;
	hash_t       *r_action;
	
	if(chain == NULL || request == NULL)
		return_error(-EINVAL, "chain_query 'chain' or 'request' is null\n");
	
	if( (r_action = hash_find_typed_value(request, TYPE_INT32, "action")) == NULL)
		return_error(-EINVAL, "chain_query request 'action' not set\n");
	
	switch(HVALUE(r_action, unsigned int)){
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
		backend_destroy(backend);
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

void         backend_destroy  (backend_t *backend){
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

/* buffer_io {{{ */
static ssize_t  backend_buffer_func_read  (buffer_t *buffer, off_t offset, void *buf, size_t buf_size){ // {{{
	buffer_t        buffer_read;
	ssize_t         ret;
	
	hash_t  hash[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ)  },
		{ "key",    DATA_PTR_OFFT(&offset)        },
		{ "size",   DATA_INT32(buf_size)          },
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_end
	};
	buffer_init_from_bare(&buffer_read, buf, buf_size);
	
	ret = chain_query( (chain_t *)buffer->io_context, hash, &buffer_read);	
	
	buffer_destroy(&buffer_read);
	return ret;
} // }}}

static ssize_t  backend_buffer_func_write (buffer_t *buffer, off_t offset, void *buf, size_t buf_size){ // {{{
	buffer_t        buffer_write;
	ssize_t         ret;
	
	hash_t  hash[] = {
		{ "action", DATA_INT32(ACTION_CRWD_WRITE) },
		{ "key",    DATA_PTR_OFFT(&offset)        },
		{ "size",   DATA_INT32(buf_size)          },
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_end
	};
	buffer_init_from_bare(&buffer_write, buf, buf_size);
	
	ret = chain_query( (chain_t *)buffer->io_context, hash, &buffer_write);	
	
	buffer_destroy(&buffer_write);
	
	return ret;
} // }}}

void            backend_buffer_io_init  (buffer_t *buffer, chain_t *chain, int cached){ // {{{
	buffer_io_init(buffer, (void *)chain, &backend_buffer_func_read, &backend_buffer_func_write, cached);
} // }}}

buffer_t *      backend_buffer_io_alloc (chain_t *chain, int cached){ // {{{
	return buffer_io_alloc((void *)chain, &backend_buffer_func_read, &backend_buffer_func_write, cached);
} // }}}

/* }}} */
