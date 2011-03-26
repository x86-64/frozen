#include <libfrozen.h>

/*
backend_t * chain_new(char *name){
	int      ret;
	backend_t *chain;
	backend_t *chain_proto = NULL;
	
	list_iter(&chains, (iter_callback)&chain_find_by_name, name, &chain_proto);
	if(chain_proto == NULL)
		return NULL;
	
	chain = (backend_t *)malloc(sizeof(backend_t));
	if(chain == NULL)
		return NULL;
	
	memcpy(chain, chain_proto, sizeof(backend_t));
	
	chain->cnext = NULL;
	
	ret = chain->func_init(chain);
	if(ret != 0){
		free(chain);
		return NULL;
	}
	
	return chain;
}
void chain_destroy(backend_t *chain){
	if(chain == NULL)
		return;

	chain->func_destroy(chain);
	
	free(chain);
}
inline int  chain_configure    (backend_t *chain, hash_t *config){ return chain->func_configure(chain, config); }

ssize_t     chain_query        (backend_t *chain, request_t *request){
	f_crwd        func       = NULL;
	ssize_t       ret;
	uint32_t      r_action;
	
	if(chain == NULL || request == NULL)
		return -ENOSYS;
	
	hash_data_copy(ret, TYPE_UINT32T, r_action, request, HK(action));
	if(ret != 0)
		return -ENOSYS;
	
	switch(r_action){
		case ACTION_CRWD_CREATE: func = chain->backend_type_crwd.func_create; break;
		case ACTION_CRWD_READ:   func = chain->backend_type_crwd.func_get   ; break;
		case ACTION_CRWD_WRITE:  func = chain->backend_type_crwd.func_set   ; break;
		case ACTION_CRWD_DELETE: func = chain->backend_type_crwd.func_delete; break;
		case ACTION_CRWD_MOVE:   func = chain->backend_type_crwd.func_move  ; break;
		case ACTION_CRWD_COUNT:  func = chain->backend_type_crwd.func_count ; break;
		case ACTION_CRWD_CUSTOM: func = chain->backend_type_crwd.func_custom; break;
		default:
			return -EINVAL;
	};	
	
	if(func == NULL)
		return chain_query(chain->cnext, request);
	
	return func(chain, request);
}

ssize_t     chain_next_query   (backend_t *chain, request_t *request){
	return chain_query(chain->cnext, request);
}

static ssize_t backend_iter_chain_init(hash_t *config, void *p_backend, void *p_chain){
	ssize_t    ret;
	backend_t   *chain;
	char      *chain_name;
	hash_t    *chain_config;
	
	if(data_value_type(hash_item_data(config)) != TYPE_HASHT)
		return ITER_BREAK;
	
	chain_config = (hash_t *) data_value_ptr(hash_item_data(config));
	
	hash_data_copy(ret, TYPE_STRING, chain_name, chain_config, HK(name));
	if(ret != 0) return ITER_BREAK;
	
	if( (chain = chain_new(chain_name)) == NULL)
		return ITER_BREAK;
	
	chain->cnext = *(backend_t **)p_chain;
	
	if(chain_configure(chain, chain_config) != 0)
		goto cleanup;
	
	*(backend_t **)p_chain = chain;
	
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
*/

backend_t *  backend_new      (hash_t *config){
	return NULL;
	/*
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
	*/
}

backend_t *  backend_find(char *name){
	backend_t  *backend = NULL;
	
	//list_iter(&backends, (iter_callback)&backend_iter_find, name, &backend);
	return backend;
}

backend_t *  backend_acquire(char *name){
	backend_t  *backend = backend_find(name);
	if(backend != NULL)
		backend->refs++;
	
	return backend;
}

ssize_t      backend_bulk_new(hash_t *config){
	//if(hash_iter(config, &backend_iter_backend_new, NULL, NULL) == ITER_BREAK){
		// TODO destory previosly allocated backends
		return -EINVAL;
	//}
	//return 0;
}

char *          backend_get_name        (backend_t *backend){
	return backend->name;
}

ssize_t      backend_query        (backend_t *backend, request_t *request){
	if(backend == NULL || request == NULL)
		return -EINVAL;
	
	return -EINVAL; // chain_query(backend->chain, request);
}
ssize_t         backend_pass            (backend_t *backend, request_t *request){
	return -EINVAL;
}

void         backend_destroy  (backend_t *backend){
	/*
	if(backend == NULL)
		return;
	
	if(--backend->refs > 0)
		return;
	
	list_unlink(&backends, backend); 
	
	backend_t *chain_curr = backend->chain;
	backend_t *chain_next;
	
	while(chain_curr != NULL){
		chain_next = chain_curr->cnext;
		chain_destroy(chain_curr);
		chain_curr = chain_next;
	}
	
	if(backend->name)
		free(backend->name);
	free(backend);
	*/
}

void        backend_destroy_all (void){
	//backend_t *backend;

	//while((backend = list_pop(&backends))){
	//	backend_destroy(backend);
	//}
}

ssize_t         backend_stdcall_create(backend_t *backend, off_t *offset, size_t size){ // {{{
	request_t  r_create[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_CREATE)     },
		{ HK(size),       DATA_PTR_SIZET(&size)              },
		{ HK(offset_out), DATA_PTR_OFFT(offset)              },
		hash_end
	};
	
	return backend_query(backend, r_create);
} // }}}
ssize_t         backend_stdcall_read  (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size){ // {{{
	request_t  r_read[] = {
		{ HK(action),  DATA_UINT32T(ACTION_CRWD_READ)   },
		{ HK(offset),  DATA_PTR_OFFT(&offset)         },
		{ HK(size),    DATA_PTR_SIZET(&buffer_size)   },
		{ HK(buffer),  DATA_RAW(buffer, buffer_size)  },
		hash_end
	};
	
	return backend_query(backend, r_read);
} // }}}
ssize_t         backend_stdcall_write (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size){ // {{{
	request_t  r_write[] = {
		{ HK(action),  DATA_UINT32T(ACTION_CRWD_WRITE)  },
		{ HK(offset),  DATA_PTR_OFFT(&offset)         },
		{ HK(size),    DATA_PTR_SIZET(&buffer_size)   },
		{ HK(buffer),  DATA_RAW(buffer, buffer_size)  },
		hash_end
	};
	
	return backend_query(backend, r_write);
} // }}}
ssize_t         backend_stdcall_fill  (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size, size_t fill_size){ // {{{
	size_t size;
	
	do{
		size = MIN(fill_size, buffer_size);
		backend_stdcall_write(backend, offset, buffer, size);
		
		offset    += size;
		fill_size -= size;
	}while(fill_size > 0);
	return 0;
} // }}}
ssize_t         backend_stdcall_delete(backend_t *backend, off_t  offset, size_t size){ // {{{
	request_t  r_delete[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_DELETE)     },
		{ HK(offset),     DATA_PTR_OFFT(&offset)             },
		{ HK(size),       DATA_PTR_SIZET(&size)              },
		hash_end
	};
	
	return backend_query(backend, r_delete);
} // }}}
ssize_t         backend_stdcall_count (backend_t *backend, size_t *count){ // {{{
	request_t r_count[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_COUNT)      },
		{ HK(buffer),     DATA_PTR_SIZET(count)              },
		hash_end
	};
	
	return backend_query(backend, r_count);
} // }}}

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

