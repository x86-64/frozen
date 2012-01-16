#define MACHINE_C
#include <libfrozen.h>

#include <pthread.h>

static list                    classes        = LIST_INITIALIZER; // dynamic classes passed from modules
static list                    machines_top   = LIST_INITIALIZER;
static list                    machines_names = LIST_INITIALIZER;
static pthread_mutex_t         refs_mtx       = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t                destroy_mtx;
pthread_key_t                  key_curr_request;

typedef struct machine_new_ctx {
	list                   machines;
	machine_t             *machine_prev;
} machine_new_ctx;

static int         class_strcmp            (char *class_str1, char *class_str2){ // {{{
	char                  *class_str1_name;
	char                  *class_str2_name;
	uintmax_t              only_names        = 0;
	
	                                  class_str1_name = strchr(class_str1, '/') + 1;
	if(class_str1_name == (char *)1)  class_str1_name = strchr(class_str1, '.') + 1;
	if(class_str1_name == (char *)1){ class_str1_name = class_str1; only_names = 1; } 
	                                  class_str2_name = strchr(class_str2, '/') + 1;
	if(class_str2_name == (char *)1)  class_str2_name = strchr(class_str2, '.') + 1;
	if(class_str2_name == (char *)1){ class_str2_name = class_str2; only_names = 1; } 
	
	if(only_names == 1){
		class_str1 = class_str1_name;
		class_str2 = class_str2_name;
	}
	
	return strcmp(class_str1, class_str2);
} // }}}
static machine_t * class_find              (char *class){ // {{{
	uintmax_t              i;
	machine_t             *machine;
	void                  *list_status  = NULL;
	
	for(i=0; i<machine_protos_size; i++){
		machine = machine_protos[i];

		if(machine->class != NULL && class_strcmp(machine->class, class) == 0)
			return machine;
	}
	
	list_rdlock(&classes);
		while( (machine = list_iter_next(&classes, &list_status)) != NULL){
			if(machine->class != NULL && class_strcmp(machine->class, class) == 0)
				goto exit;
		}
		machine = NULL;
exit:
	list_unlock(&classes);
	
	return machine;
} // }}}
ssize_t            class_register          (machine_t *proto){ // {{{
	if(
		proto->class == NULL
	)
		return -EINVAL;
	
	list_add(&classes, proto);
	return 0;
} // }}}
void               class_unregister        (machine_t *proto){ // {{{
	list_delete(&classes, proto);
} // }}}

request_t *        request_get_current(void){ // {{{
	return (request_t *)pthread_getspecific(key_curr_request);
} // }}}

ssize_t            thread_data_init(thread_data_ctx_t *thread_data, f_thread_create func_create, f_thread_destroy func_destroy, void *userdata){ // {{{
	if(thread_data->inited == 0){
		if(pthread_key_create(&thread_data->key, func_destroy) != 0)
			return -ENOMEM;
		
		thread_data->inited       = 1;
		thread_data->func_create  = func_create;
		thread_data->userdata     = userdata;
	}
	return 0;
} // }}}
void               thread_data_destroy(thread_data_ctx_t *thread_data){ // {{{
	if(thread_data->inited == 1){
		pthread_key_delete(thread_data->key);
		thread_data->inited = 0;
	}
} // }}}
void *             thread_data_get(thread_data_ctx_t *thread_data){ // {{{
	register void         *data;
	
	if( (data = pthread_getspecific(thread_data->key)) != NULL)
		return data;
	
	data = thread_data->func_create(thread_data->userdata);
	pthread_setspecific(thread_data->key, data);
	
	return data;
} // }}}

static void        machine_ref_inc(machine_t *machine){ // {{{
	pthread_mutex_lock(&refs_mtx);
		machine->refs++;
	pthread_mutex_unlock(&refs_mtx);
} // }}}
static uintmax_t   machine_ref_dec(machine_t *machine){ // {{{
	uintmax_t              ret;
	pthread_mutex_lock(&refs_mtx);
		ret = --machine->refs;
	pthread_mutex_unlock(&refs_mtx);
	return ret;
} // }}}

static void        machine_top(machine_t *machine){ // {{{
	if(list_is_empty(&machine->parents)){
		list_delete (&machines_top, machine);
		list_add    (&machines_top, machine);
	}
} // }}}
static void        machine_untop(machine_t *machine){ // {{{
	list_delete(&machines_top, machine);
} // }}}

static void        machine_free_skeleton(machine_t *machine_curr){ // {{{
	machine_untop(machine_curr);
		
	hash_free(machine_curr->config);
	
	if(machine_curr->name){
		list_delete(&machines_names, machine_curr);
		free(machine_curr->name);
	}
	
	list_destroy(&machine_curr->childs);
	list_destroy(&machine_curr->parents);
	
	free(machine_curr);
} // }}}
static machine_t * machine_new_skeleton(char *machine_name, char *machine_class, hash_t *config){ // {{{
	machine_t             *machine;
	machine_t             *class;
	
	if( (class = class_find(machine_class)) == NULL)
		goto error;
	
	if( (machine = machine_clone(class)) == NULL)
		goto error;
	
	if(machine_name){
		if( (machine->name = strdup(machine_name)) == NULL)
			goto error_mem;
		
		list_add(&machines_names, machine);
	}else{
		machine->name = NULL;
	}
	
	if( (machine->config = hash_copy(config)) == NULL)
		goto error_mem;
	
	machine->userdata   = NULL;
	return machine;
	
error_mem:
	machine_free_skeleton(machine);
error:
	return NULL;
} // }}}

static ssize_t     machine_new_iterator(hash_t *item, machine_new_ctx *ctx){ // {{{
	ssize_t                ret;
	machine_t             *machine;
	hash_t                *machine_cfg;
	data_t                *machine_cfg_data;
	char                  *machine_name;
	char                  *machine_class;
	
	if(item->key == hash_ptr_null){
		ctx->machine_prev = LIST_FREE_ITEM;
		return ITER_CONTINUE;
	}
		
	machine_cfg_data = &item->data;
	
	if(machine_cfg_data->type != TYPE_HASHT)
		return ITER_CONTINUE;
	
	machine_cfg = (hash_t *)machine_cfg_data->ptr;
	
	hash_data_copy(ret, TYPE_STRINGT, machine_name,  machine_cfg, HK(name));  if(ret != 0) machine_name  = NULL;
	hash_data_copy(ret, TYPE_STRINGT, machine_class, machine_cfg, HK(class)); if(ret != 0) machine_class = NULL;
	
	if(machine_class == NULL && machine_name == NULL)
		goto error_inval;
	
	if(machine_class == NULL){
		if( (machine = machine_find(machine_name)) == NULL)
			goto error_inval;
		
		machine_connect(machine, ctx->machine_prev);
	}else{
		if( (machine = machine_new_skeleton(machine_name, machine_class, machine_cfg)) == NULL)
			goto error_inval;

		machine_connect(machine, ctx->machine_prev);
		
		if(machine->func_init != NULL && machine->func_init(machine) != 0)
			goto error_init;
		
		if(machine->func_configure != NULL && machine->func_configure(machine, machine->config) != 0)
			goto error_configure;
		
		list_add(&ctx->machines, machine);
	}
	
	ctx->machine_prev = machine;
	return ITER_CONTINUE;
	
error_configure:
	if(machine->func_destroy != NULL)
		machine->func_destroy(machine);
	
error_init:
	machine_disconnect(machine, ctx->machine_prev);
	machine_free_skeleton(machine);

error_inval:
	return ITER_BREAK;
} // }}}
static machine_t * machine_fork_rec(machine_t *machine, request_t *request){ // {{{
	uintmax_t              lsz, i;
	machine_t             *machine_curr;
	void                 **childs_list;
	
	list_rdlock(&machine->childs);
		
		if( (lsz = list_count(&machine->childs)) != 0){
			childs_list = (void **)alloca( sizeof(void *) * lsz );
			list_flatten(&machine->childs, childs_list, lsz);
		}
		
	list_unlock(&machine->childs);
	
	for(i=0; i<lsz; i++){
		if( (childs_list[i] = machine_fork_rec((machine_t *)childs_list[i], request)) == NULL){
			// unwind
			while(i > 0){
				i--;
				machine_destroy(childs_list[i]);
			}
			
			return NULL;
		}
	}
	
	if( (machine_curr = machine_clone(machine)) == NULL)
		goto error_inval;
		
	// NOTE forked machine lose name and parents
	machine_curr->name     = NULL;
	machine_curr->userdata = NULL;
	
	for(i=0; i<lsz; i++)
		machine_connect(machine_curr, childs_list[i]);
	
	if(machine_curr->func_init != NULL && machine_curr->func_init(machine_curr) != 0)
		goto error_init;
	
	if(machine_curr->func_fork != NULL){
		if(machine_curr->func_fork(machine_curr, machine, request) != 0)
			goto error_configure;
	}else{
		if(machine_curr->func_configure != NULL && machine_curr->func_configure(machine_curr, machine_curr->config) != 0)
			goto error_configure;
	}
	
	return machine_curr;
	
error_configure:
	if(machine_curr->func_destroy != NULL)
		machine_curr->func_destroy(machine_curr);
	
error_init:
	for(i=0; i<lsz; i++)
		machine_disconnect(machine_curr, childs_list[i]);
	
	machine_free_skeleton(machine_curr);
	
error_inval:
	return NULL;
} // }}}
static ssize_t     machine_downgrade_request(machine_t *machine, fastcall_header *hargs){ // {{{
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
			if( (b_ret = machine_query(machine, r_create)) < 0)
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
			if( (b_ret = machine_query(machine, r_io)) < 0)
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
			if( (b_ret = machine_query(machine, r_delete)) < 0)
				return b_ret;
			break;
		
		case ACTION_COUNT:;
			request_t  r_count[] = {
				{ HK(action),     DATA_PTR_UINT32T( &hargs->action )                      },
				{ HK(buffer),     DATA_PTR_UINTT( &((fastcall_count *)hargs)->nelements ) },
				{ HK(ret),        DATA_PTR_SIZET(&q_ret)                                  },
				hash_end
			};
			if( (b_ret = machine_query(machine, r_count)) < 0)
				return b_ret;
			break;
		default:
			return -ENOSYS;
	};
	return q_ret;
} // }}}

void               machine_connect(machine_t *parent, machine_t *child){ // {{{
	list_add(&parent->childs, child);
	machine_top(parent);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	machine_untop(child);
	list_add(&child->parents, parent);
} // }}}
void               machine_disconnect(machine_t *parent, machine_t *child){ // {{{
	machine_untop(parent);
	list_delete(&parent->childs, child);
	
	if(child == LIST_FREE_ITEM)
		return;
	
	list_delete(&child->parents, parent);
	machine_top(child);
} // }}}
void               machine_add_terminators(machine_t *machine, list *new_childs){ // {{{
	uintmax_t              lsz;
	machine_t             *child;
	void                  *childs_iter = NULL;
	
	list_rdlock(&machine->childs); // TODO loop shops bugs
		
		if( (lsz = list_count(&machine->childs)) != 0){
			// go recurse
			while( (child = list_iter_next(&machine->childs, &childs_iter)) != NULL)
				machine_add_terminators(child, new_childs);
		}
	
	list_unlock(&machine->childs);
		
	if(lsz == 0){
		// add new childs to terminating machines without childs
		while( (child = list_iter_next(new_childs, &childs_iter)) != NULL)
			machine_connect(machine, child);
	}
} // }}}
void               machine_insert(machine_t *parent, machine_t *new_child){ // {{{
	machine_t             *child;
	
	machine_add_terminators(new_child, &parent->childs);
	
	while( (child = list_pop(&parent->childs)) != NULL)
		machine_disconnect(parent, child);
	
	machine_connect(parent, new_child);
} // }}}
	
ssize_t            frozen_machine_init(void){ // {{{
	pthread_mutexattr_t    attr;
	
	if(pthread_mutexattr_init(&attr) != 0)
		return -EFAULT;
		
	if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		return -EFAULT;
	
	if(pthread_mutex_init(&destroy_mtx, &attr) != 0)
		return -EFAULT;
	
	pthread_mutexattr_destroy(&attr);
	
	if(pthread_key_create(&key_curr_request, NULL) != 0)
		return -EFAULT;
	
	return 0;
} // }}}
void               frozen_machine_destroy(void){ // {{{
	machine_t             *machine;
	
	while( (machine = list_pop(&machines_top)) != NULL)
		machine_destroy(machine);
	
	list_destroy(&machines_top);
	list_destroy(&machines_names);
	
	pthread_mutex_destroy(&destroy_mtx);
	pthread_key_delete(key_curr_request);
} // }}}

machine_t *     machine_new          (hash_t *config){ // {{{
	machine_t             *machine;
	machine_t             *child;
	machine_new_ctx        ctx               = { LIST_INITIALIZER, LIST_FREE_ITEM };
	
	if(hash_iter(config, (hash_iterator)&machine_new_iterator, &ctx, HASH_ITER_NULL) == ITER_OK){
		list_destroy(&ctx.machines);
		
		if(ctx.machine_prev == LIST_FREE_ITEM)
			return NULL;
		
		return ctx.machine_prev;
	}
	
	while( (machine = list_pop(&ctx.machines)) != NULL){
		if(machine->func_destroy != NULL)
			machine->func_destroy(machine);
		
		while( (child = list_pop(&machine->childs)) != NULL)
			machine_disconnect(machine, child);

		machine_free_skeleton(machine);
	}
	return NULL;
} // }}}
machine_t *     machine_find         (char *name){ // {{{
	machine_t             *machine;
	void                  *list_status  = NULL;
	
	if(name == NULL)
		return NULL;
	
	list_rdlock(&machines_names);
		while( (machine = list_iter_next(&machines_names, &list_status)) != NULL){
			if(machine->name == NULL)
				continue;
			if(strcmp(machine->name, name) == 0)
				goto exit;
		}
		machine = NULL;
exit:
	list_unlock(&machines_names);
	return machine;
} // }}}
void            machine_acquire      (machine_t *machine){ // {{{
	if(machine)
		machine_ref_inc(machine);
} // }}}
machine_t *     machine_fork         (machine_t *machine, request_t *request){ // {{{
	static request_t       r_fork[] = { hash_end };
	
	if(machine == NULL)
		return NULL;
	
	if(request == NULL)
		request = r_fork;
	
	return machine_fork_rec(machine, request);
} // }}}
machine_t *     machine_clone        (machine_t *machine){ // {{{
	machine_t *clone;
	
	if(machine == NULL)
		return NULL;
	
	if( (clone = malloc(sizeof(machine_t))) == NULL)
		return NULL;
	
	memcpy(clone, machine, sizeof(machine_t));
	
	list_init(&clone->parents);
	list_init(&clone->childs);
	
	clone->refs     = 1;
	clone->config   = hash_copy(machine->config);
	
	return clone;
} // }}}
void            machine_destroy      (machine_t *machine){ // {{{
	machine_t             *curr;
	
	if(machine == NULL)
		return;
	
	pthread_mutex_lock(&destroy_mtx);
		if(machine_ref_dec(machine) == 0){
			// call destroy
			if(machine->func_destroy != NULL)
				machine->func_destroy(machine);
			
			while( (curr = list_pop(&machine->childs)) != NULL)   // recursive destroy of all left childs
				machine_destroy(curr);
			
			while( (curr = list_pop(&machine->parents)) != NULL)  // remove this machine from parents's childs list
				machine_disconnect(curr, machine);
			
			machine_free_skeleton(machine);
		}
	pthread_mutex_unlock(&destroy_mtx);
} // }}}

ssize_t         machine_query        (machine_t *machine, request_t *request){ // {{{
	f_crwd                 func              = NULL;
	ssize_t                ret;
	data_t                 ret_data          = DATA_PTR_SIZET(&ret);
	data_t                *ret_req;
	
	if(machine == NULL || request == NULL)
		return -ENOSYS;
	
	do {
		if( (machine->supported_api & API_HASH) != 0){
			func = machine->machine_type_hash.func_handler;
			break;
		}
		
		if( (machine->supported_api & API_CRWD) != 0){
			uint32_t               r_action;
			
			hash_data_copy(ret, TYPE_UINT32T, r_action, request, HK(action)); if(ret != 0) return -ENOSYS;
			
			switch(r_action){
				case ACTION_CREATE: func = machine->machine_type_crwd.func_create; break;
				case ACTION_READ:   func = machine->machine_type_crwd.func_get   ; break;
				case ACTION_WRITE:  func = machine->machine_type_crwd.func_set   ; break;
				case ACTION_DELETE: func = machine->machine_type_crwd.func_delete; break;
				case ACTION_MOVE:   func = machine->machine_type_crwd.func_move  ; break;
				case ACTION_COUNT:  func = machine->machine_type_crwd.func_count ; break;
				case ACTION_CUSTOM: func = machine->machine_type_crwd.func_custom; break;
				default:
					return -EINVAL;
			};
			break;
		}
	}while(0);
	
	if(func == NULL){
		ret = machine_pass(machine, request);
		goto exit;
	}
	
	pthread_setspecific(key_curr_request, request);
	
	ret = func(machine, request);
	
	switch(ret){
		case -EBADF:            // dead machine
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
ssize_t         machine_pass         (machine_t *machine, request_t *request){ // {{{
	ssize_t                ret = 0;
	ssize_t                ret2;
	uintmax_t              lsz, i;
	void                 **childs_list;
	
	list_rdlock(&machine->childs);
		
		if( (lsz = list_count(&machine->childs)) != 0){
			childs_list = (void **)alloca( sizeof(void *) * lsz );
			list_flatten(&machine->childs, childs_list, lsz);
		}
		
	list_unlock(&machine->childs);
	
	if(lsz == 0)
		return -ENOSYS;
	
	for(i=0; i<lsz; i++){
		if( (ret2 = machine_query((machine_t *)childs_list[i], request)) < 0){
			ret = ret2;
		}
	}
	return ret;
} // }}}
ssize_t         machine_fast_query   (machine_t *machine, void *fargs){ // {{{
	f_fast_func            func              = NULL;
	
	if(machine == NULL || fargs == NULL)
		return -ENOSYS;
	
	do {
		if( (machine->supported_api & API_FAST) != 0){
			func = machine->machine_type_fast.func_handler;
			break;
		}
		// TODO API_FAST_TABLE
		
		// if fast api not supported by machine - downgrade fast request to hash request and do query
		return machine_downgrade_request(machine, fargs);
	}while(0);
	
	if(func == NULL)
		return machine_fast_pass(machine, fargs);
	
	return func(machine, fargs);
} // }}}
ssize_t         machine_fast_pass    (machine_t *machine, void *fargs){ // {{{
	ssize_t                ret = 0;
	uintmax_t              lsz, i;
	void                 **childs_list;
	
	list_rdlock(&machine->childs);
		
		if( (lsz = list_count(&machine->childs)) != 0){
			childs_list = (void **)alloca( sizeof(void *) * lsz );
			list_flatten(&machine->childs, childs_list, lsz);
		}
		
	list_unlock(&machine->childs);
	
	if(lsz == 0)
		return -ENOSYS;
	
	for(i=0; i<lsz; i++){
		if( (ret = machine_fast_query((machine_t *)childs_list[i], fargs)) < 0)
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
