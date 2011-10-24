#define BACKEND_C
#include <libfrozen.h>
#include <backend_selected.h>

#include <alloca.h>
#include <pthread.h>

static list                    classes        = LIST_INITIALIZER; // dynamic classes passed from modules
static list                    backends_top   = LIST_INITIALIZER;
static list                    backends_names = LIST_INITIALIZER;
pthread_mutex_t                refs_mtx       = PTHREAD_MUTEX_INITIALIZER;

typedef struct backend_new_ctx {
	list                   backends;
	backend_t             *backend_prev;
} backend_new_ctx;

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
	pthread_mutex_lock(&refs_mtx);
		backend->refs++;
	pthread_mutex_unlock(&refs_mtx);
} // }}}
static uintmax_t   backend_ref_dec(backend_t *backend){ // {{{
	uintmax_t              ret;
	pthread_mutex_lock(&refs_mtx);
		ret = --backend->refs;
	pthread_mutex_unlock(&refs_mtx);
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

static void        backend_free_skeleton(backend_t *backend_curr){ // {{{
	hash_free(backend_curr->config);
	
	if(backend_curr->name){
		list_delete(&backends_names, backend_curr);
		free(backend_curr->name);
	}
	
	list_destroy(&backend_curr->childs);
	list_destroy(&backend_curr->parents);
	
	free(backend_curr);
} // }}}
static backend_t * backend_new_skeleton(char *backend_name, char *backend_class, hash_t *config){ // {{{
	backend_t             *backend;
	backend_t             *class;
	
	if( (class = class_find(backend_class)) == NULL)
		goto error;
	
	if( (backend = backend_clone(class)) == NULL)
		goto error;
	
	if(backend_name){
		if( (backend->name = strdup(backend_name)) == NULL)
			goto error_mem;
		
		list_add(&backends_names, backend);
	}else{
		backend->name = NULL;
	}
	
	if( (backend->config = hash_copy(config)) == NULL)
		goto error_mem;
	
	backend->userdata   = NULL;
	return backend;
	
error_mem:
	backend_free_skeleton(backend);
error:
	return NULL;
} // }}}

static ssize_t     backend_new_iterator(hash_t *item, backend_new_ctx *ctx){ // {{{
	ssize_t                ret;
	backend_t             *backend;
	hash_t                *backend_cfg;
	data_t                *backend_cfg_data;
	char                  *backend_name;
	char                  *backend_class;
	
	if(item->key == hash_ptr_null){
		ctx->backend_prev = LIST_FREE_ITEM;
		return ITER_CONTINUE;
	}
		
	backend_cfg_data = &item->data;
	
	if(backend_cfg_data->type != TYPE_HASHT)
		return ITER_CONTINUE;
	
	backend_cfg = (hash_t *)backend_cfg_data->ptr;
	
	hash_data_copy(ret, TYPE_STRINGT, backend_name,  backend_cfg, HK(name));  if(ret != 0) backend_name  = NULL;
	hash_data_copy(ret, TYPE_STRINGT, backend_class, backend_cfg, HK(class)); if(ret != 0) backend_class = NULL;
	
	if(backend_class == NULL && backend_name == NULL)
		goto error_inval;
	
	if(backend_class == NULL){
		if( (backend = backend_find(backend_name)) == NULL)
			goto error_inval;
		
		backend_connect(backend, ctx->backend_prev);
	}else{
		if( (backend = backend_new_skeleton(backend_name, backend_class, backend_cfg)) == NULL)
			goto error_inval;

		backend_connect(backend, ctx->backend_prev);
		
		if(backend->func_init != NULL && backend->func_init(backend) != 0)
			goto error_init;
		
		if(backend->func_configure != NULL && backend->func_configure(backend, backend->config) != 0)
			goto error_configure;
	}
	
	ctx->backend_prev = backend;
	list_add(&ctx->backends, backend);
	return ITER_CONTINUE;
	
error_configure:
	if(backend->func_destroy != NULL)
		backend->func_destroy(backend);
	
error_init:
	backend_disconnect(backend, ctx->backend_prev);
	backend_free_skeleton(backend);

error_inval:
	return ITER_BREAK;
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
	
	backend_free_skeleton(backend_curr);
	
error_inval:
	return NULL;
} // }}}
static ssize_t     backend_downgrade_request(backend_t *backend, fastcall_header *hargs){ // {{{
	ssize_t q_ret = 0, b_ret;
	
	switch(hargs->action){
		case ACTION_CREATE:;
			request_t  r_create[] = {
				{ HK(action),     DATA_PTR_UINT32T( &hargs->action )                    },
				{ HK(size),       DATA_PTR_UINTT( &((fastcall_create *)hargs)->size   ) },
				{ HK(offset_out), DATA_PTR_UINTT( &((fastcall_create *)hargs)->offset ) },
				{ HK(ret),        DATA_PTR_SIZET(&q_ret)                                },
				hash_end
			};
			if( (b_ret = backend_query(backend, r_create)) < 0)
				return b_ret;
			break;

		case ACTION_READ:
		case ACTION_WRITE:;
			request_t  r_io[] = {
				{ HK(action),     DATA_PTR_UINT32T( &hargs->action )                       },
				{ HK(offset),     DATA_PTR_UINTT( &((fastcall_io *)hargs)->offset        ) },
				{ HK(buffer),     DATA_RAW( ((fastcall_io *)hargs)->buffer, ((fastcall_io *)hargs)->buffer_size ) },
				{ HK(size),       DATA_PTR_UINTT( &((fastcall_io *)hargs)->buffer_size   ) },
				{ HK(ret),        DATA_PTR_SIZET(&q_ret)                                   },
				hash_end
			};
			if( (b_ret = backend_query(backend, r_io)) < 0)
				return b_ret;
			break;
		
		case ACTION_DELETE:;
			request_t  r_delete[] = {
				{ HK(action),     DATA_PTR_UINT32T( &hargs->action )                    },
				{ HK(offset),     DATA_PTR_UINTT( &((fastcall_delete *)hargs)->offset ) },
				{ HK(size),       DATA_PTR_UINTT( &((fastcall_delete *)hargs)->size   ) },
				{ HK(ret),        DATA_PTR_SIZET(&q_ret)                                },
				hash_end
			};
			if( (b_ret = backend_query(backend, r_delete)) < 0)
				return b_ret;
			break;
		
		case ACTION_COUNT:;
			request_t  r_count[] = {
				{ HK(action),     DATA_PTR_UINT32T( &hargs->action )                      },
				{ HK(buffer),     DATA_PTR_UINTT( &((fastcall_count *)hargs)->nelements ) },
				{ HK(ret),        DATA_PTR_SIZET(&q_ret)                                  },
				hash_end
			};
			if( (b_ret = backend_query(backend, r_count)) < 0)
				return b_ret;
			break;
		default:
			return -ENOSYS;
	};
	return q_ret;
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
void               backend_add_terminators(backend_t *backend, list *new_childs){ // {{{
	uintmax_t              lsz;
	backend_t             *child;
	void                  *childs_iter = NULL;
	
	list_rdlock(&backend->childs); // TODO loop chains bugs
		
		if( (lsz = list_count(&backend->childs)) != 0){
			// go recurse
			while( (child = list_iter_next(&backend->childs, &childs_iter)) != NULL)
				backend_add_terminators(child, new_childs);
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
	
	backend_add_terminators(new_child, &parent->childs);
	
	while( (child = list_pop(&parent->childs)) != NULL)
		backend_disconnect(parent, child);
	
	backend_connect(parent, new_child);
} // }}}

backend_t *     backend_new          (hash_t *config){ // {{{
	backend_t             *backend;
	backend_t             *child;
	backend_new_ctx        ctx               = { LIST_INITIALIZER, LIST_FREE_ITEM };
	
	if(hash_iter(config, (hash_iterator)&backend_new_iterator, &ctx, HASH_ITER_NULL) == ITER_OK){
		if(ctx.backend_prev == LIST_FREE_ITEM)
			return NULL;
		
		return ctx.backend_prev;
	}
	
	while( (backend = list_pop(&ctx.backends)) != NULL){
		if(backend->func_destroy != NULL)
			backend->func_destroy(backend);
		
		while( (child = list_pop(&backend->childs)) != NULL)
			backend_disconnect(backend, child);

		backend_free_skeleton(backend);
	}
	return NULL;
} // }}}
backend_t *     backend_find         (char *name){ // {{{
	backend_t             *backend;
	void                  *list_status  = NULL;
	
	if(name == NULL)
		return NULL;
	
	list_rdlock(&backends_names);
		while( (backend = list_iter_next(&backends_names, &list_status)) != NULL){
			if(backend->name == NULL)
				continue;
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
	
	clone->refs     = 1;
	clone->config   = hash_copy(backend->config);
	
	return clone;
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
	
	free(backend);
} // }}}
void            backend_destroy_all  (void){ // {{{
	backend_t             *backend;
	
	while( (backend = list_pop(&backends_top)) != NULL)
		backend_destroy(backend);
	
	list_destroy(&backends_top);
	list_destroy(&backends_names);
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
				case ACTION_CREATE: func = backend->backend_type_crwd.func_create; break;
				case ACTION_READ:   func = backend->backend_type_crwd.func_get   ; break;
				case ACTION_WRITE:  func = backend->backend_type_crwd.func_set   ; break;
				case ACTION_DELETE: func = backend->backend_type_crwd.func_delete; break;
				case ACTION_MOVE:   func = backend->backend_type_crwd.func_move  ; break;
				case ACTION_COUNT:  func = backend->backend_type_crwd.func_count ; break;
				case ACTION_CUSTOM: func = backend->backend_type_crwd.func_custom; break;
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
ssize_t         backend_fast_query   (backend_t *backend, void *fargs){ // {{{
	f_fast_func            func              = NULL;
	
	if(backend == NULL || fargs == NULL)
		return -ENOSYS;
	
	do {
		if( (backend->supported_api & API_FAST) != 0){
			func = backend->backend_type_fast.func_handler;
			break;
		}
		// TODO API_FAST_TABLE
		
		// if fast api not supported by backend - downgrade fast request to hash request and do query
		return backend_downgrade_request(backend, fargs);
	}while(0);
	
	if(func == NULL)
		return backend_fast_pass(backend, fargs);
	
	return func(backend, fargs);
} // }}}
ssize_t         backend_fast_pass    (backend_t *backend, void *fargs){ // {{{
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
		if( (ret = backend_fast_query((backend_t *)childs_list[i], fargs)) < 0)
			return ret;
	}
	return 0;
} // }}}

data_functions request_str_to_action(char *string){ // {{{
	if(strcasecmp(string, "create") == 0) return ACTION_CREATE;
	if(strcasecmp(string, "write")  == 0) return ACTION_WRITE;
	if(strcasecmp(string, "read")   == 0) return ACTION_READ;
	if(strcasecmp(string, "delete") == 0) return ACTION_DELETE;
	if(strcasecmp(string, "move")   == 0) return ACTION_MOVE;
	if(strcasecmp(string, "count")  == 0) return ACTION_COUNT;
	if(strcasecmp(string, "custom") == 0) return ACTION_CUSTOM;
	if(strcasecmp(string, "compare") == 0) return ACTION_COMPARE;
	return ACTION_INVALID;
} // }}}
