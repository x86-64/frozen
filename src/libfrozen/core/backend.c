#define BACKEND_C
#include <libfrozen.h>
#include <backend_selected.h>

#define LIST_FREE_ITEM (void *)-1
#define LIST_END       (void *)NULL

static backend_t             **top_backends      = NULL;

static backend_t *  backend_find_class (char *class){ // {{{
	uintmax_t i;
	
	for(i=0; i<backend_protos_size; i++){
		if(strcmp(backend_protos[i]->class, class) == 0)
			return backend_protos[i];
	}
	
	return NULL;
} // }}}
static void         backend_list_add(backend_t ***p_list, backend_t *item){ // {{{
	backend_t   **list;
	backend_t    *curr;
	size_t        lsz      = 0;
	
	if( (list = *p_list) != NULL){
		while( (curr = list[lsz]) != LIST_END){
			if(curr == LIST_FREE_ITEM)
				goto found;
			
			lsz++;
		}
	}
	list = *p_list = realloc(list, sizeof(backend_t *) * (lsz + 2));
	list[lsz + 1] = LIST_END;
found:
	list[lsz + 0] = item;
} // }}}
static void         backend_list_delete(backend_t ***p_list, backend_t *item){ // {{{
	backend_t   **list;
	backend_t    *curr;
	size_t        lsz;
	
	if( (list = *p_list) == NULL)
		return;
	
	lsz  = 0;
	while( (curr = list[lsz]) != LIST_END){
		if(curr == item){
			list[lsz] = LIST_FREE_ITEM;
			return;
		}
		
		lsz++;
	}
} // }}}
static intmax_t     backend_list_is_empty(backend_t ***p_list){ // {{{
	backend_t   **list;
	backend_t    *curr;
	size_t        lsz;
	
	if( (list = *p_list) == NULL)
		return 1;
	
	lsz  = 0;
	while( (curr = list[lsz]) != LIST_END){
		if(curr != LIST_FREE_ITEM)
			return 0;
		
		lsz++;
	}
	return 1;
} // }}}
static backend_t *  backend_list_next(backend_t ***p_list){ // {{{
	backend_t    *curr;
	backend_t   **list;
	size_t        lsz;
	
	if( (list = *p_list) == NULL)
		return NULL;
	
	lsz  = 0;
	while( (curr = list[lsz]) != LIST_END){
		if(curr != LIST_FREE_ITEM){
			*p_list = &list[lsz+1];
			return curr;
		}
		
		lsz++;
	}
	*p_list = NULL;
	return NULL;
} // }}}

static void        backend_top(backend_t *backend){ // {{{
	if(backend_list_is_empty(&backend->parents)){
			
			backend_list_delete(&top_backends, backend);
			backend_list_add(&top_backends, backend);
		
	}
} // }}}
static void        backend_untop(backend_t *backend){ // {{{
		
		backend_list_delete(&top_backends, backend);
	
} // }}}
static void        backend_connect(backend_t *parent, backend_t *child){ // {{{
	pthread_rwlock_wrlock(&parent->rwlock);
		backend_list_add(&parent->childs, child);
		parent->refs++;
		
		backend_top(parent);
	pthread_rwlock_unlock(&parent->rwlock);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	pthread_rwlock_wrlock(&child->rwlock);
		backend_untop(child);
		
		backend_list_add(&child->parents, parent);
		child->refs++;
	pthread_rwlock_unlock(&child->rwlock);
} // }}}
static void        backend_disconnect(backend_t *parent, backend_t *child){ // {{{
	pthread_rwlock_wrlock(&parent->rwlock);
		backend_untop(parent);
	
		backend_list_delete(&parent->childs, child);
		parent->refs--;
	pthread_rwlock_unlock(&parent->rwlock);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	pthread_rwlock_wrlock(&child->rwlock);
		backend_list_delete(&child->parents, parent);
		child->refs--;
		
		backend_top(child);
	pthread_rwlock_unlock(&child->rwlock);
} // }}}
static backend_t * backend_new_rec(hash_t *config, backend_t *backend_prev){ // {{{
	ssize_t                ret;
	backend_t             *backend_curr, *backend_last, *class;
	hash_t                *backend_cfg;
	data_t                *backend_cfg_data;
	char                  *backend_name;
	char                  *backend_class;
	
	if(hash_item_is_null(config)){
		backend_curr = LIST_FREE_ITEM;
		backend_prev = LIST_FREE_ITEM;
		goto recurse;
	}
		
	backend_cfg_data = hash_item_data(config);
	
	if(data_value_type(backend_cfg_data) != TYPE_HASHT)
		goto error_inval;
	
	backend_cfg = GET_TYPE_HASHT(backend_cfg_data);
	
	hash_data_copy(ret, TYPE_STRINGT, backend_name,  backend_cfg, HK(name));  if(ret != 0) backend_name  = NULL;
	hash_data_copy(ret, TYPE_STRINGT, backend_class, backend_cfg, HK(class)); if(ret != 0) backend_class = NULL;
	
	if(backend_class == NULL && backend_name == NULL)
		goto error_inval;
	
	if(backend_class == NULL){
		if( (backend_curr = backend_find(backend_name)) == NULL)
			goto error_inval;
		
		backend_connect(backend_curr, backend_prev);
	}else{
		if( (backend_curr = malloc(sizeof(backend_t))) == NULL)
			goto error_malloc_backend;
		if( (class = backend_find_class(backend_class)) == NULL)
			goto error_class;
		
		memcpy(backend_curr, class, sizeof(backend_t));
		
		if( (backend_curr->parents = malloc(sizeof(backend_t *) * (1 + 1))) == NULL)
			goto error_malloc_parent;
		if( (backend_curr->childs  = malloc(sizeof(backend_t *) * (1 + 1))) == NULL)
			goto error_malloc_child;
		
		pthread_rwlock_init(&backend_curr->rwlock, NULL);
		backend_curr->name       = backend_name ? strdup(backend_name) : NULL;
		backend_curr->refs       = 0;
		backend_curr->userdata   = NULL;
		
		backend_curr->parents[0] = LIST_FREE_ITEM;
		backend_curr->parents[1] = LIST_END;
		backend_curr->childs[0]  = LIST_FREE_ITEM;
		backend_curr->childs[1]  = LIST_END;
		
		backend_connect(backend_curr, backend_prev);
		
		if(backend_curr->func_init(backend_curr) != 0)
			goto error_init;
		
		if(backend_curr->func_configure(backend_curr, backend_cfg) != 0)
			goto error_configure;
	}

recurse:
	if( (config = hash_item_next(config)) == NULL)
		return backend_curr;
	
	if( (backend_last = backend_new_rec(config, backend_curr)) != NULL)
		return backend_last;
	
	if( backend_curr == LIST_FREE_ITEM)
		return NULL;
	
error_configure:
	backend_curr->func_destroy(backend_curr);

error_init:
	backend_disconnect(backend_curr, backend_prev);
	
	if(backend_curr->name)
		free(backend_curr->name);
	
	pthread_rwlock_destroy(&backend_curr->rwlock);
	free(backend_curr->childs);
	
error_malloc_child:
	free(backend_curr->parents);

error_malloc_parent:
error_class:
	free(backend_curr);
	
error_malloc_backend:
error_inval:
	return NULL;
} // }}}

backend_t *  backend_new          (hash_t *config){ // {{{
	return backend_new_rec(config, LIST_FREE_ITEM);
} // }}}
backend_t *  backend_find         (char *name){ // {{{
	backend_t  *backend = NULL;
	
	//list_iter(&backends, (iter_callback)&backend_iter_find, name, &backend);
	return backend;
} // }}}
backend_t *  backend_acquire      (char *name){ // {{{
	backend_t  *backend = backend_find(name);
	if(backend != NULL)
		backend->refs++;
	
	return backend;
} // }}}
char *       backend_get_name     (backend_t *backend){ // {{{
	return backend->name;
} // }}}
ssize_t      backend_query        (backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint32_t               r_action;
	f_crwd                 func              = NULL;
	data_t                 ret_data          = DATA_PTR_SIZET(&ret);
	data_t                *ret_req;
	data_ctx_t            *ret_req_ctx;
	
	if(backend == NULL || request == NULL)
		return -ENOSYS;
	
	hash_data_copy(ret, TYPE_UINT32T, r_action, request, HK(action)); if(ret != 0) return -ENOSYS;
	
	switch(r_action){
		case ACTION_CRWD_CREATE: func = backend->backend_type_crwd.func_create; break;
		case ACTION_CRWD_READ:   func = backend->backend_type_crwd.func_get   ; break;
		case ACTION_CRWD_WRITE:  func = backend->backend_type_crwd.func_set   ; break;
		case ACTION_CRWD_DELETE: func = backend->backend_type_crwd.func_delete; break;
		case ACTION_CRWD_MOVE:   func = backend->backend_type_crwd.func_move  ; break;
		case ACTION_CRWD_COUNT:  func = backend->backend_type_crwd.func_count ; break;
		case ACTION_CRWD_CUSTOM: func = backend->backend_type_crwd.func_custom; break;
		default:
			return -EINVAL;
	};	
	if(func == NULL)
		return backend_pass(backend, request);
	
	ret = func(backend, request);
	
	if(ret != -EEXIST){
		hash_data_find(request, HK(ret), &ret_req, &ret_req_ctx);
		data_transfer(ret_req, ret_req_ctx, &ret_data, NULL);
	}
	return 0;
} // }}}
ssize_t      backend_pass         (backend_t *backend, request_t *request){ // {{{
	ssize_t                ret = 0;
	uintmax_t              lsz = 0;
	backend_t             *backend_next;
	
	pthread_rwlock_rdlock(&backend->rwlock);
		while( (backend_next = backend->childs[lsz++]) != LIST_END){
			if(backend_next == LIST_FREE_ITEM)
				continue;
			
			if( (ret = backend_query(backend_next, request)) < 0)
				goto error;
		}
error:
	pthread_rwlock_unlock(&backend->rwlock);
	return ret;
} // }}}
void         backend_destroy      (backend_t *backend){ // {{{
	backend_t             *curr;
	backend_t            **list_cpy;
	
	if(backend == NULL)
		return;
	
	// NOTE BUG bad things can happen if someone _acquire backend, _destory it, and _query
	
	pthread_rwlock_wrlock(&backend->rwlock);
		
		// call destroy
		backend->func_destroy(backend);
		
		// recursive destory of all left childs
		list_cpy = backend->childs;
		while( (curr = backend_list_next(&list_cpy)) != NULL){
			backend_destroy(curr);
		}
		
		// remove this backend from parents's childs list
		list_cpy = backend->parents;
		while( (curr = backend_list_next(&list_cpy)) != NULL){
			backend_disconnect(curr, backend);
		}
		
		backend_untop(backend);
		
	pthread_rwlock_unlock(&backend->rwlock);
	pthread_rwlock_destroy(&backend->rwlock);
	
	// free memory
	if(backend->name)
		free(backend->name);
	free(backend->childs);
	free(backend->parents);
	free(backend);
} // }}}
void         backend_destroy_all  (void){ // {{{
	backend_t  *backend;
	backend_t **top_backends_cpy;
	
	
		if( (top_backends_cpy = top_backends) == NULL)
			return;
		
		while( (backend = backend_list_next(&top_backends_cpy)) != NULL)
			backend_destroy(backend);
		
		free(top_backends);
		top_backends = NULL;
	
} // }}}

ssize_t         backend_stdcall_create(backend_t *backend, off_t *offset, size_t size){ // {{{
	ssize_t q_ret = 0, b_ret;
	
	request_t  r_create[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_CREATE)   },
		{ HK(size),       DATA_PTR_SIZET(&size)              },
		{ HK(offset_out), DATA_PTR_OFFT(offset)              },
		{ HK(ret),        DATA_PTR_SIZET(&q_ret)             },
		hash_end
	};
	if( (b_ret = backend_query(backend, r_create)) < 0)
		return b_ret;
	
	return q_ret;
} // }}}
ssize_t         backend_stdcall_read  (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size){ // {{{
	ssize_t q_ret = 0, b_ret;
	
	request_t  r_read[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_READ)    },
		{ HK(offset),     DATA_PTR_OFFT(&offset)            },
		{ HK(size),       DATA_PTR_SIZET(&buffer_size)      },
		{ HK(buffer),     DATA_RAW(buffer, buffer_size)     },
		{ HK(ret),        DATA_PTR_SIZET(&q_ret)            },
		hash_end
	};
	
	if( (b_ret = backend_query(backend, r_read)) < 0)
		return b_ret;
	
	return q_ret;
} // }}}
ssize_t         backend_stdcall_write (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size){ // {{{
	ssize_t q_ret = 0, b_ret;
	
	request_t  r_write[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_WRITE)   },
		{ HK(offset),     DATA_PTR_OFFT(&offset)            },
		{ HK(size),       DATA_PTR_SIZET(&buffer_size)      },
		{ HK(buffer),     DATA_RAW(buffer, buffer_size)     },
		{ HK(ret),        DATA_PTR_SIZET(&q_ret)            },
		hash_end
	};
	
	if( (b_ret = backend_query(backend, r_write)) < 0)
		return b_ret;
	
	return q_ret;
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
ssize_t         backend_stdcall_move  (backend_t *backend, off_t  from, off_t to, size_t size){ // {{{
	ssize_t q_ret = 0, b_ret;
	
	request_t  r_move[] = {
		{ HK(action),      DATA_UINT32T(ACTION_CRWD_MOVE)       },
		{ HK(offset_from), DATA_PTR_OFFT(&from)                 },
		{ HK(offset_to),   DATA_PTR_OFFT(&to)                   },
		{ HK(size),        DATA_PTR_SIZET(&size)                },
		{ HK(ret),         DATA_PTR_SIZET(&q_ret)               },
		hash_end
	};
	
	if( (b_ret = backend_query(backend, r_move)) < 0)
		return b_ret;
	
	return q_ret;
} // }}}
ssize_t         backend_stdcall_delete(backend_t *backend, off_t  offset, size_t size){ // {{{
	ssize_t q_ret = 0, b_ret;
	
	request_t  r_delete[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_DELETE)     },
		{ HK(offset),     DATA_PTR_OFFT(&offset)               },
		{ HK(size),       DATA_PTR_SIZET(&size)                },
		{ HK(ret),        DATA_PTR_SIZET(&q_ret)               },
		hash_end
	};
	
	if( (b_ret = backend_query(backend, r_delete)) < 0)
		return b_ret;
	
	return q_ret;
} // }}}
ssize_t         backend_stdcall_count (backend_t *backend, size_t *count){ // {{{
	ssize_t q_ret = 0, b_ret;
	
	request_t r_count[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_COUNT)      },
		{ HK(buffer),     DATA_PTR_SIZET(count)                },
		{ HK(ret),        DATA_PTR_SIZET(&q_ret)               },
		hash_end
	};
	
	if( (b_ret = backend_query(backend, r_count)) < 0)
		return b_ret;
	
	return q_ret;
} // }}}

request_actions request_str_to_action(char *string){ // {{{
	if(strcasecmp(string, "create") == 0) return ACTION_CRWD_CREATE;
	if(strcasecmp(string, "write")  == 0) return ACTION_CRWD_WRITE;
	if(strcasecmp(string, "read")   == 0) return ACTION_CRWD_READ;
	if(strcasecmp(string, "delete") == 0) return ACTION_CRWD_DELETE;
	if(strcasecmp(string, "move")   == 0) return ACTION_CRWD_MOVE;
	if(strcasecmp(string, "count")  == 0) return ACTION_CRWD_COUNT;
	if(strcasecmp(string, "custom") == 0) return ACTION_CRWD_CUSTOM;
	return REQUEST_INVALID;
} // }}}

