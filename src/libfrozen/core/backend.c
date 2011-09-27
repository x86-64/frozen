#define BACKEND_C
#include <libfrozen.h>
#include <alloca.h>
#include <backend_selected.h>

static list                    classes        = LIST_INITIALIZER; // dynamic classes passed from modules
static list                    backends_top   = LIST_INITIALIZER;
static list                    backends_names = LIST_INITIALIZER;

static int         class_strcmp            (char *class_str1, char *class_str2){ // {{{
	char                  *class_str1_name;
	char                  *class_str2_name;
	uintmax_t              only_names        = 0;
	
	                                  class_str1_name = index(class_str1, '/') + 1;
	if(class_str1_name == (char *)1)  class_str1_name = index(class_str1, '.') + 1;
	if(class_str1_name == (char *)1){ class_str1_name = class_str1; only_names = 1; } 
	                                  class_str2_name = index(class_str2, '/') + 1;
	if(class_str2_name == (char *)1)  class_str2_name = index(class_str2, '.') + 1;
	if(class_str2_name == (char *)1){ class_str2_name = class_str2; only_names = 1; } 
	
	if(only_names == 1){
		class_str1 = class_str1_name;
		class_str2 = class_str2_name;
	}
	
	return strcmp(class_str1, class_str2);
} // }}}
static backend_t * class_find              (char *class){ // {{{
	uintmax_t              i;
	backend_t             *backend;
	void                  *list_status  = NULL;
	
	for(i=0; i<backend_protos_size; i++){
		backend = backend_protos[i];

		if(backend->class != NULL && class_strcmp(backend->class, class) == 0)
			return backend;
	}
	
	list_rdlock(&classes);
		while( (backend = list_iter_next(&classes, &list_status)) != NULL){
			if(backend->class != NULL && class_strcmp(backend->class, class) == 0)
				goto exit;
		}
		backend = NULL;
exit:
	list_unlock(&classes);
	
	return backend;
} // }}}
ssize_t            class_register          (backend_t *proto){ // {{{
	if(
		proto->class == NULL
	)
		return -EINVAL;
	
	list_add(&classes, proto);
	return 0;
} // }}}
void               class_unregister        (backend_t *proto){ // {{{
	list_delete(&classes, proto);
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
void               backend_connect(backend_t *parent, backend_t *child){ // {{{
	list_add(&parent->childs, child);
	backend_top(parent);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	backend_untop(child);
	list_add(&child->parents, parent);
} // }}}
void               backend_disconnect(backend_t *parent, backend_t *child){ // {{{
	backend_untop(parent);
	list_delete(&parent->childs, child);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	list_delete(&child->parents, parent);
	backend_top(child);
} // }}}
void               backend_insert_rec(backend_t *backend, list *new_childs){ // {{{
	uintmax_t              lsz;
	backend_t             *child;
	void                  *childs_iter = NULL;
	
	list_rdlock(&backend->childs); // TODO loop chains bugs
		
		if( (lsz = list_count(&backend->childs)) != 0){
			// go recurse
			while( (child = list_iter_next(&backend->childs, &childs_iter)) != NULL)
				backend_insert_rec(child, new_childs);
		}
	
	list_unlock(&backend->childs);
		
	if(lsz == 0){
		// add new childs to terminating backends without childs
		while( (child = list_iter_next(new_childs, &childs_iter)) != NULL)
			backend_connect(backend, child);
	}
} // }}}
void               backend_insert(backend_t *parent, backend_t *new_child){ // {{{
	backend_t             *child;
	
	backend_insert_rec(new_child, &parent->childs);
	
	while( (child = list_pop(&parent->childs)) != NULL)
		backend_disconnect(parent, child);
	
	backend_connect(parent, new_child);
} // }}}
static void        backend_free_from_proto(backend_t *backend_curr){ // {{{
	hash_free(backend_curr->config);
	
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
	
	if(backend_cfg_data->type != TYPE_HASHT)
		goto error_inval;
	
	backend_cfg = (hash_t *)backend_cfg_data->ptr;
	
	hash_data_convert_ptr(ret, TYPE_STRINGT, backend_name,  backend_cfg, HK(name));  if(ret != 0) backend_name  = NULL;
	hash_data_convert_ptr(ret, TYPE_STRINGT, backend_class, backend_cfg, HK(class)); if(ret != 0) backend_class = NULL;
	
	if(backend_class == NULL && backend_name == NULL)
		goto error_inval;
	
	if(backend_class == NULL){
		if( (backend_curr = backend_find(backend_name)) == NULL)
			goto error_inval;
		
		backend_connect(backend_curr, backend_prev);
	}else{
		if( (class = class_find(backend_class)) == NULL)
			goto error_inval;
		
		if( (backend_curr = backend_clone(class)) == NULL)
			goto error_inval;
	
		if(backend_name){
			backend_curr->name = strdup(backend_name);
			list_add(&backends_names, backend_curr);
		}else{
			backend_curr->name = NULL;
		}
		
		backend_curr->config     = hash_copy(backend_cfg);
		backend_curr->userdata   = NULL;
		backend_connect(backend_curr, backend_prev);
		
		if(backend_curr->func_init != NULL && backend_curr->func_init(backend_curr) != 0)
			goto error_init;
		
		if(backend_curr->func_configure != NULL && backend_curr->func_configure(backend_curr, backend_curr->config) != 0)
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
	if(backend_curr->func_destroy != NULL)
		backend_curr->func_destroy(backend_curr);
	
error_init:
	backend_disconnect(backend_curr, backend_prev);
	
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
	
	if( (backend_curr = backend_clone(backend)) == NULL)
		goto error_inval;
		
	// NOTE forked backend lose name and parents
	backend_curr->name     = NULL;
	backend_curr->userdata = NULL;
	
	for(i=0; i<lsz; i++)
		backend_connect(backend_curr, childs_list[i]);
	
	if(backend_curr->func_init != NULL && backend_curr->func_init(backend_curr) != 0)
		goto error_init;
	
	if(backend_curr->func_fork != NULL){
		if(backend_curr->func_fork(backend_curr, backend, request) != 0)
			goto error_configure;
	}else{
		if(backend_curr->func_configure != NULL && backend_curr->func_configure(backend_curr, backend_curr->config) != 0)
			goto error_configure;
	}
	
	return backend_curr;
	
error_configure:
	if(backend_curr->func_destroy != NULL)
		backend_curr->func_destroy(backend_curr);
	
error_init:
	for(i=0; i<lsz; i++)
		backend_disconnect(backend_curr, childs_list[i]);
	
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
backend_t *     backend_clone        (backend_t *backend){ // {{{
	backend_t *clone;
	
	if( (clone = malloc(sizeof(backend_t))) == NULL)
		return NULL;
	
	memcpy(clone, backend, sizeof(backend_t));
	
	list_init(&clone->parents);
	list_init(&clone->childs);
	pthread_mutex_init(&clone->refs_mtx, NULL);
	
	clone->refs     = 1;
	clone->config   = hash_copy(backend->config);
	
	return clone;
} // }}}
ssize_t         backend_query        (backend_t *backend, request_t *request){ // {{{
	f_crwd                 func              = NULL;
	ssize_t                ret;
	data_t                 ret_data          = DATA_PTR_SIZET(&ret);
	data_t                *ret_req;
	
	if(backend == NULL || request == NULL)
		return -ENOSYS;
	
	do {
		if( (backend->supported_api & API_HASH) != 0){
			func = backend->backend_type_hash.func_handler;
			break;
		}
		
		if( (backend->supported_api & API_CRWD) != 0){
			uint32_t               r_action;
			
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
			break;
		}
	}while(0);
	
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
			ret_req = hash_data_find(request, HK(ret));
			fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, ret_req };
			data_query(&ret_data, &r_transfer);
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
	if(backend->func_destroy != NULL)
		backend->func_destroy(backend);
	
	while( (curr = list_pop(&backend->childs)) != NULL)   // recursive destroy of all left childs
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
/*backend_t *     backend_from_data  (data_t *data){ // {{{
	ssize_t                ret;
	hash_t                *backend_config    = NULL;
	char                  *backend_name      = NULL;

	switch( data_value_type(data) ){
		case TYPE_HASHT:
			data_to_dt(ret, TYPE_HASHT,   backend_config, data, NULL);
			if(ret == 0)
				return backend_new(backend_config);
			break;
		case TYPE_STRINGT:
			data_to_dt(ret, TYPE_STRINGT, backend_name, data, NULL);
			if(ret == 0)
				return backend_find(backend_name);
			break;
		default:
			return NULL;
	}
	return NULL;
} */

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
