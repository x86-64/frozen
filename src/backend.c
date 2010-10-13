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

/* crwd_fastcall {{{
int           fc_crwd_init    (crwd_fastcall *fc_table){
	int  action;
	
	memset(fc_table, 0, sizeof(crwd_fastcall));
	
	if( (fc_table->request_create = hash_new()) == NULL)  goto free;
	if( (fc_table->request_read   = hash_new()) == NULL)  goto free;
	if( (fc_table->request_write  = hash_new()) == NULL)  goto free;
	if( (fc_table->request_move   = hash_new()) == NULL)  goto free;
	if( (fc_table->request_delete = hash_new()) == NULL)  goto free;
	if( (fc_table->request_count  = hash_new()) == NULL)  goto free;
	if( (fc_table->request_custom = hash_new()) == NULL)  goto free;
	
	if( (fc_table->buffer_create = buffer_alloc()) == NULL) goto free;
	if( (fc_table->buffer_read   = buffer_alloc()) == NULL) goto free;
	if( (fc_table->buffer_write  = buffer_alloc()) == NULL) goto free;
	if( (fc_table->buffer_count  = buffer_alloc()) == NULL) goto free;
	if( (fc_table->buffer_custom = buffer_alloc()) == NULL) goto free;
	
	action = ACTION_CRWD_CREATE; hash_set(fc_table->request_create, "action", TYPE_INT32, &action);
	action = ACTION_CRWD_READ;   hash_set(fc_table->request_read,   "action", TYPE_INT32, &action);
	action = ACTION_CRWD_WRITE;  hash_set(fc_table->request_write,  "action", TYPE_INT32, &action);
	action = ACTION_CRWD_MOVE;   hash_set(fc_table->request_move,   "action", TYPE_INT32, &action);
	action = ACTION_CRWD_DELETE; hash_set(fc_table->request_delete, "action", TYPE_INT32, &action);
	action = ACTION_CRWD_COUNT;  hash_set(fc_table->request_count,  "action", TYPE_INT32, &action);
	action = ACTION_CRWD_CUSTOM; hash_set(fc_table->request_custom, "action", TYPE_INT32, &action);
	
	return 0;
free:
	fc_crwd_destroy(fc_table);
	return -1;
}

void          fc_crwd_destroy (crwd_fastcall *fc_table){
	if( (fc_table->request_create != NULL)) hash_free(fc_table->request_create);
	if( (fc_table->request_read   != NULL)) hash_free(fc_table->request_read);
	if( (fc_table->request_write  != NULL)) hash_free(fc_table->request_write);
	if( (fc_table->request_move   != NULL)) hash_free(fc_table->request_move);
	if( (fc_table->request_delete != NULL)) hash_free(fc_table->request_delete);
	if( (fc_table->request_count  != NULL)) hash_free(fc_table->request_count);
	if( (fc_table->request_custom != NULL)) hash_free(fc_table->request_custom);
	
	if( (fc_table->buffer_create  != NULL)) buffer_free(fc_table->buffer_create);
	if( (fc_table->buffer_read    != NULL)) buffer_free(fc_table->buffer_read);
	if( (fc_table->buffer_write   != NULL)) buffer_free(fc_table->buffer_write);
	if( (fc_table->buffer_count   != NULL)) buffer_free(fc_table->buffer_count);
	if( (fc_table->buffer_custom  != NULL)) buffer_free(fc_table->buffer_custom);
	
	memset(fc_table, 0, sizeof(crwd_fastcall));
}

static ssize_t  fc_crwd_call  (crwd_fastcall *fc_table, chain_t *chain, va_list args){ // {{{ 
	int           action;
	ssize_t       ret;
	off_t         r_key;
	off_t         r_key_from;
	off_t         r_key_to;
	unsigned int  r_size;
	void         *r_buf;
	char         *r_fname;
	buffer_t    **o_buffer;
	void         *o_buf;
	size_t        o_buf_size;
	
	action = va_arg(args, int);
	switch(action){
		case ACTION_CRWD_CREATE: // (fc_table, chain, ACTION_CRWD_CREATE, size, &key, sizeof(key)); // {{{
			r_size     = va_arg(args, unsigned int);
			o_buf      = va_arg(args, void *);
			o_buf_size = va_arg(args, size_t);
			
			if(hash_set(fc_table->request_create, "size", TYPE_INT32, &r_size)) return -1;
			
			ret = chain_query(chain, fc_table->request_create, fc_table->buffer_create);
			if(ret > 0){
				buffer_read(fc_table->buffer_create, 0, o_buf, MIN(ret, o_buf_size)); 
			}
			return ret;
		// }}}
		case ACTION_CRWD_READ:   // (fc_table, chain, ACTION_CRWD_READ, key, &buffer, size); // {{{
			r_key      = va_arg(args, off_t);
			o_buffer   = va_arg(args, buffer_t **);
			r_size     = va_arg(args, unsigned int);
			
			if(hash_set(fc_table->request_read, "key",  TYPE_INT64, &r_key)  != 0) return -1;
			if(hash_set(fc_table->request_read, "size", TYPE_INT32, &r_size) != 0) return -1;
			
			ret = chain_query(chain, fc_table->request_read, fc_table->buffer_read);
			if(ret > 0){
				*o_buffer = fc_table->buffer_read;
			}
			return ret;
		// }}}
		case ACTION_CRWD_WRITE:  // (fc_table, chain, ACTION_CRWD_WRITE, key, buf, size); // {{{
			r_key      = va_arg(args, off_t);
			r_buf      = va_arg(args, void *);
			r_size     = va_arg(args, unsigned int);
			
			if(hash_set        (fc_table->request_write, "key",    TYPE_INT64, &r_key)      != 0) return -1;
			if(hash_set        (fc_table->request_write, "size",   TYPE_INT32, &r_size)     != 0) return -1;
			
			buffer_write(fc_table->buffer_write, 0, r_buf, r_size);
			
			return chain_query(chain, fc_table->request_write, fc_table->buffer_write);
		
		// }}}
		case ACTION_CRWD_DELETE: // (fc_table, chain, ACTION_CRWD_DELETE, key, size); // {{{
			r_key      = va_arg(args, off_t);
			r_size     = va_arg(args, unsigned int);
			
			if(hash_set        (fc_table->request_delete, "key",   TYPE_INT64, &r_key)      != 0) return -1;
			if(hash_set        (fc_table->request_delete, "size",  TYPE_INT32, &r_size)     != 0) return -1;
			
			return chain_query(chain, fc_table->request_delete, NULL);
		// }}}
		case ACTION_CRWD_MOVE:   // (fc_table, chain, ACTION_CRWD_MOVE, key_from, key_to, size); // {{{
			r_key_from = va_arg(args, off_t);
			r_key_to   = va_arg(args, off_t);
			r_size     = va_arg(args, unsigned int);
			
			if(hash_set        (fc_table->request_move, "key_from", TYPE_INT64, &r_key_from) != 0) return -1;
			if(hash_set        (fc_table->request_move, "key_to",   TYPE_INT64, &r_key_to)   != 0) return -1;
			if(hash_set        (fc_table->request_move, "size",     TYPE_INT32, &r_size)     != 0) return -1;
			
			return chain_query(chain, fc_table->request_move, NULL);
		// }}}
		case ACTION_CRWD_COUNT:  // (fc_table, chain, ACTION_CRWD_COUNT, &size, sizeof(size)); // {{{
			o_buf      = va_arg(args, void *);
			o_buf_size = va_arg(args, size_t);
			
			ret = chain_query(chain, fc_table->request_count, fc_table->buffer_count);
			if(ret > 0){
				buffer_read(fc_table->buffer_count, 0, o_buf, MIN(ret, o_buf_size)); 
			}
			return ret;
		// }}}
		case ACTION_CRWD_CUSTOM: // (fc_table, chain, ACTION_CRWD_CUSTOM, "func_name", [&buf], [sizeof(buf)]); // {{{
			r_fname    = va_arg(args, char *);
			o_buf      = va_arg(args, void *);
			o_buf_size = va_arg(args, size_t);
			
			if(hash_set       (fc_table->request_custom, "function", TYPE_STRING, &r_fname) != 0) return -1;
			
			ret = chain_query(chain, fc_table->request_custom, fc_table->buffer_custom);
			if(ret > 0 && o_buf != NULL){
				buffer_read(fc_table->buffer_custom, 0, o_buf, MIN(ret, o_buf_size)); 
			}
			return ret;
		// }}}
		default:
			return -EINVAL;
	};	
} // }}}

ssize_t         fc_crwd_chain      (crwd_fastcall *fc_table, ...){ // {{{ 
	ssize_t  ret;
	va_list  args;
	chain_t *chain;
	
	va_start(args, fc_table);
	
	chain = va_arg(args, chain_t *);
	
	ret = fc_crwd_call(fc_table, chain, args);
	
	va_end(args);
	return ret;
} // }}}
ssize_t         fc_crwd_next_chain (crwd_fastcall *fc_table, ...){ // {{{ 
	ssize_t  ret;
	va_list  args;
	chain_t *chain;
	
	va_start(args, fc_table);
	
	chain = va_arg(args, chain_t *);
	
	ret = fc_crwd_call(fc_table, chain->next, args);
	
	va_end(args);
	return ret;
} // }}}
ssize_t         fc_crwd_backend    (crwd_fastcall *fc_table, ...){ // {{{ 
	ssize_t    ret;
	va_list    args;
	backend_t *backend;
	
	va_start(args, fc_table);
	
	backend = va_arg(args, backend_t *);
	
	ret = fc_crwd_call(fc_table, backend->chain, args);
	
	va_end(args);
	return ret;
} // }}}

 }}} */

/* buffer_io {{{ */
static ssize_t  backend_buffer_func_read  (buffer_t *buffer, off_t offset, void *buf, size_t buf_size){ // {{{
	buffer_t        buffer_read;
	ssize_t         ret;
	
	hash_t  hash[] = {
		{ TYPE_INT32, "action", (int []){ACTION_CRWD_READ} },
		{ TYPE_INT32, "key",    &offset                    },
		{ TYPE_INT32, "size",   &buf_size                  },
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
		{ TYPE_INT32, "action", (int []){ACTION_CRWD_WRITE}},
		{ TYPE_INT32, "key",    &offset                    },
		{ TYPE_INT32, "size",   &buf_size                  },
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
