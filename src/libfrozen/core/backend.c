#define BACKEND_C
#include <libfrozen.h>
#include <alloca.h>
#include <backend_selected.h>

static list                    backends_top   = LIST_INITIALIZER;
static list                    backends_names = LIST_INITIALIZER;

static backend_t * backend_find_class (char *class){ // {{{
	uintmax_t i;
	
	for(i=0; i<backend_protos_size; i++){
		if(strcmp(backend_protos[i]->class, class) == 0)
			return backend_protos[i];
	}
	
	return NULL;
} // }}}
static void        backend_ref_inc(backend_t *backend){ // {{{
	pthread_mutex_lock(&backend->refs_mtx);
		backend->refs++;
	pthread_mutex_unlock(&backend->refs_mtx);
} // }}}
static uintmax_t   backend_ref_dec(backend_t *backend){ // {{{
	uintmax_t              ret;
	pthread_mutex_lock(&backend->refs_mtx);
		ret = --backend->refs;
	pthread_mutex_unlock(&backend->refs_mtx);
	return ret;
} // }}}
static void        backend_top(backend_t *backend){ // {{{
	if(list_is_empty(&backend->parents)){
		list_delete (&backends_top, backend);
		list_add    (&backends_top, backend);
	}
} // }}}
static void        backend_untop(backend_t *backend){ // {{{
	list_delete(&backends_top, backend);
} // }}}
static void        backend_connect(backend_t *parent, backend_t *child){ // {{{
	list_add(&parent->childs, child);
	backend_top(parent);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	backend_untop(child);
	list_add(&child->parents, parent);
} // }}}
static void        backend_disconnect(backend_t *parent, backend_t *child){ // {{{
	backend_untop(parent);
	list_delete(&parent->childs, child);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	list_delete(&child->parents, parent);
	backend_top(child);
} // }}}
static backend_t * backend_copy_from_proto(backend_t *proto, char *backend_name){ // {{{
	backend_t *backend_curr;
	
	if( (backend_curr = malloc(sizeof(backend_t))) == NULL)
		return NULL;
	
	memcpy(backend_curr, proto, sizeof(backend_t));
	
	list_init(&backend_curr->parents);
	list_init(&backend_curr->childs);
	pthread_mutex_init(&backend_curr->refs_mtx, NULL);
	
	if(backend_name){
		backend_curr->name = strdup(backend_name);
		list_add(&backends_names, backend_curr);
	}else{
		backend_curr->name = NULL;
	}
	backend_curr->refs       = 1;
	backend_curr->userdata   = NULL;
	
	return backend_curr;
} // }}}
static void        backend_free_from_proto(backend_t *backend_curr){ // {{{
	if(backend_curr->name){
		list_delete(&backends_names, backend_curr);
		free(backend_curr->name);
	}
	
	pthread_mutex_destroy(&backend_curr->refs_mtx);
	list_destroy(&backend_curr->childs);
	list_destroy(&backend_curr->parents);
	
	free(backend_curr);
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
		if( (class = backend_find_class(backend_class)) == NULL)
			goto error_inval;
		
		if( (backend_curr = backend_copy_from_proto(class, backend_name)) == NULL)
			goto error_inval;
		
		backend_curr->config     = hash_copy(backend_cfg);
		backend_connect(backend_curr, backend_prev);
		
		if(backend_curr->func_init(backend_curr) != 0)
			goto error_init;
		
		if(backend_curr->func_configure(backend_curr, backend_curr->config) != 0)
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
	hash_free(backend_curr->config);
	
	backend_free_from_proto(backend_curr);

error_inval:
	return NULL;
} // }}}
static backend_t * backend_fork_rec(backend_t *backend, request_t *request){ // {{{
	uintmax_t              lsz, i;
	backend_t             *backend_curr;
	void                 **childs_list;
	
	list_rdlock(&backend->childs);
		
		if( (lsz = list_count(&backend->childs)) != 0){
			childs_list = (void **)alloca( sizeof(void *) * lsz );
			list_flatten(&backend->childs, childs_list, lsz);
		}
		
	list_unlock(&backend->childs);
	
	for(i=0; i<lsz; i++){
		if( (childs_list[i] = backend_fork_rec((backend_t *)childs_list[i], request)) == NULL){
			// unwind
			while(i > 0){
				i--;
				backend_destroy(childs_list[i]);
			}
			
			return NULL;
		}
	}
	
	// NOTE forked backend lose name and parents
	if( (backend_curr = backend_copy_from_proto(backend, NULL)) == NULL)
		goto error_inval;
		
	backend_curr->config = hash_copy(backend->config);
	
	for(i=0; i<lsz; i++)
		backend_connect(backend_curr, childs_list[i]);
	
	if(backend_curr->func_init(backend_curr) != 0)
		goto error_init;
	
	if(backend_curr->func_fork != NULL){
		if(backend_curr->func_fork(backend_curr, backend, request) != 0)
			goto error_configure;
	}else{
		if(backend_curr->func_configure(backend_curr, backend_curr->config) != 0)
			goto error_configure;
	}
	
	return backend_curr;
	
error_configure:
	backend_curr->func_destroy(backend_curr);
	
error_init:
	for(i=0; i<lsz; i++)
		backend_disconnect(backend_curr, childs_list[i]);
	
	hash_free(backend_curr->config);
	
	backend_free_from_proto(backend_curr);
	
error_inval:
	return NULL;
} // }}}

backend_t *     backend_new          (hash_t *config){ // {{{
	return backend_new_rec(config, LIST_FREE_ITEM);
} // }}}
backend_t *     backend_find         (char *name){ // {{{
	backend_t             *backend;
	void                  *list_status  = NULL;
	
	if(name == NULL)
		return NULL;
	
	list_rdlock(&backends_names);
		while( (backend = list_iter_next(&backends_names, &list_status)) != NULL){
			if(strcmp(backend->name, name) == 0)
				goto exit;
		}
		backend = NULL;
exit:
	list_unlock(&backends_names);
	return backend;
} // }}}
backend_t *     backend_acquire      (char *name){ // {{{
	backend_t             *backend;
	
	if( (backend = backend_find(name)) != NULL)
		backend_ref_inc(backend);
	
	return backend;
} // }}}
backend_t *     backend_fork         (backend_t *backend, request_t *request){ // {{{
	static request_t       r_fork[] = { hash_end };
	
	if(request == NULL)
		request = r_fork;
	
	return backend_fork_rec(backend, request);
} // }}}
char *          backend_get_name     (backend_t *backend){ // {{{
	return backend->name;
} // }}}
ssize_t         backend_query        (backend_t *backend, request_t *request){ // {{{
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
	if(func == NULL){
		ret = backend_pass(backend, request);
		goto exit;
	}
	
	ret = func(backend, request);
	
	switch(ret){
		case -EBADF:            // dead backend
			return ret;
		case -EEXIST:           // no ret override
			break;
		default:                // override ret
			hash_data_find(request, HK(ret), &ret_req, &ret_req_ctx);
			data_transfer(ret_req, ret_req_ctx, &ret_data, NULL);
			break;
	};
	ret = 0;
exit:
	return ret;
} // }}}
ssize_t         backend_pass         (backend_t *backend, request_t *request){ // {{{
	ssize_t                ret = 0;
	uintmax_t              lsz, i;
	void                 **childs_list;
	
	list_rdlock(&backend->childs);
		
		if( (lsz = list_count(&backend->childs)) != 0){
			childs_list = (void **)alloca( sizeof(void *) * lsz );
			list_flatten(&backend->childs, childs_list, lsz);
		}
		
	list_unlock(&backend->childs);
	
	if(lsz == 0)
		return -ENOSYS;
	
	for(i=0; i<lsz; i++){
		if( (ret = backend_query((backend_t *)childs_list[i], request)) < 0)
			return ret;
	}
	return 0;
} // }}}
void            backend_destroy      (backend_t *backend){ // {{{
	backend_t             *curr;
	
	if(backend == NULL || backend_ref_dec(backend) != 0)
		return;
	
	// call destroy
	backend->func_destroy(backend);
	
	while( (curr = list_pop(&backend->childs)) != NULL)   // recursive destory of all left childs
		backend_destroy(curr);
	
	while( (curr = list_pop(&backend->parents)) != NULL)  // remove this backend from parents's childs list
		backend_disconnect(curr, backend);
	
	backend_untop(backend);
	
	// free memory
	if(backend->name){
		list_delete(&backends_names, backend);
		free(backend->name);
	}
	
	hash_free(backend->config);
	
	list_destroy(&backend->parents);
	list_destroy(&backend->childs);
	
	pthread_mutex_destroy(&backend->refs_mtx);
	free(backend);
} // }}}
void            backend_destroy_all  (void){ // {{{
	backend_t             *backend;
	
	while( (backend = list_pop(&backends_top)) != NULL)
		backend_destroy(backend);
	
	list_destroy(&backends_top);
	list_destroy(&backends_names);
} // }}}

ssize_t         backend_stdcall_create(backend_t *backend, off_t *offset, size_t size){ // {{{
	if( (backend->supported_api & API_FAST) != 0){
		return backend->backend_type_fast.func_fast_create(backend, offset, size);
	}else{
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
	}
} // }}}
ssize_t         backend_stdcall_read  (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size){ // {{{
	if( (backend->supported_api & API_FAST) != 0){
		return backend->backend_type_fast.func_fast_read(backend, offset, buffer, buffer_size);
	}else{
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
	}
} // }}}
ssize_t         backend_stdcall_write (backend_t *backend, off_t  offset, void *buffer, size_t buffer_size){ // {{{
	if( (backend->supported_api & API_FAST) != 0){
		return backend->backend_type_fast.func_fast_write(backend, offset, buffer, buffer_size);
	}else{
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
	}
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
	if( (backend->supported_api & API_FAST) != 0){
		return backend->backend_type_fast.func_fast_delete(backend, offset, size);
	}else{
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
	}
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

// BACKEND_FAST_PASS {{{
#define BACKEND_FAST_PASS(_backend, _action) {                                   \
	ssize_t                ret = 0;                                          \
	uintmax_t              lsz, i;                                           \
	void                 **childs_list;                                      \
	backend_t             *child;                                            \
	                                                                         \
	list_rdlock(&(_backend)->childs);                                        \
		                                                                 \
		if( (lsz = list_count(&(_backend)->childs)) != 0){               \
			childs_list = (void **)alloca( sizeof(void *) * lsz );   \
			list_flatten(&(_backend)->childs, childs_list, lsz);     \
		}                                                                \
		                                                                 \
	list_unlock(&(_backend)->childs);                                        \
	                                                                         \
	if(lsz == 0)                                                             \
		return 0;                                                        \
	                                                                         \
	for(i=0; i<lsz; i++){                                                    \
	        child = (backend_t *)childs_list[i];                             \
		ret   = _action;                                                 \
		if( ret == 0 )                                                   \
			break;                                                   \
	}                                                                        \
	return ret;                                                              \
}
// }}}
size_t          backend_pass_fast_create(backend_t *backend, off_t *offset, size_t size){ // {{{
	BACKEND_FAST_PASS(backend,
		backend_stdcall_create(child, offset, size)
	);
} // }}} 
size_t          backend_pass_fast_read(backend_t *backend, off_t offset, void *buffer, size_t buffer_size){ // {{{
	BACKEND_FAST_PASS(backend,
		backend_stdcall_read(child, offset, buffer, buffer_size)
	);
} // }}} 
size_t          backend_pass_fast_write(backend_t *backend, off_t offset, void *buffer, size_t buffer_size){ // {{{
	BACKEND_FAST_PASS(backend,
		backend_stdcall_write(child, offset, buffer, buffer_size)
	);
} // }}} 
size_t          backend_pass_fast_delete(backend_t *backend, off_t offset, size_t size){ // {{{
	BACKEND_FAST_PASS(backend,
		backend_stdcall_delete(child, offset, size)
	);
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

