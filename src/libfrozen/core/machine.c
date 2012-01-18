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
	machine_t             *machine_next;
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

static ssize_t     request_downgrade(machine_t *machine, fastcall_header *hargs){ // {{{
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
	list_delete (&machines_top, machine);
	list_add    (&machines_top, machine);
} // }}}
static void        machine_untop(machine_t *machine){ // {{{
	list_delete(&machines_top, machine);
} // }}}
static void        machine_connect(machine_t *parent, machine_t *child){ // {{{
	if(parent){
		parent->cnext = child;
		machine_top(parent);
	}
	if(child){
		child->cprev = parent;
		machine_untop(child);
	}
} // }}}
static void        machine_disconnect(machine_t *parent, machine_t *child){ // {{{
	if(parent){
		machine_untop(parent);
		parent->cnext = NULL;
	}
	if(child){
		machine_top(child);
		child->cprev  = NULL;
	}
} // }}}

static machine_t * machine_ghost_new(char *name){ // {{{
	machine_t             *machine;
	
	// create new ghost
	if( (machine = calloc(1, sizeof(machine_t))) == NULL)
		return NULL;
	
	if(name){
		if( (machine->name = strdup(name)) == NULL)
			goto error_mem;
		
		list_add(&machines_names, machine);
	}else{
		machine->name = NULL;
	}
	
	machine->refs     = 1;
	machine->userdata = NULL;
	return machine;

error_mem:
	free(machine);
	return NULL;
} // }}}
static ssize_t     machine_ghost_resurrect(machine_t *machine, machine_t *class, hash_t *config){ // {{{
	char                  *name = machine->name;
	uintmax_t              refs = machine->refs;
	
	memcpy(machine, class, sizeof(machine_t)); // TODO remove this
	machine->name = name;
	machine->refs = refs;
	
	if( (machine->config = hash_copy(config)) == NULL)
		return -ENOMEM;
	
	if(machine->func_init != NULL && machine->func_init(machine) != 0)
		return -EFAULT;
	
	if(machine->func_configure != NULL && machine->func_configure(machine, machine->config) != 0)
		goto error;
	
	return 0;
	
error:
	if(machine->func_destroy != NULL)
		machine->func_destroy(machine);
	
	return -EFAULT;
} // }}}
static void        machine_ghost_free(machine_t *machine){ // {{{
	machine_untop(machine);
	
	if(machine->name){
		list_delete(&machines_names, machine);
		free(machine->name);
	}
	free(machine);
} // }}}

ssize_t            machine_new(machine_t **pmachine, hash_t *config){ // {{{
	ssize_t                ret;
	machine_t             *class;
	machine_t             *machine;
	char                  *machine_name      = NULL;
	char                  *machine_class     = NULL;
	
	hash_data_copy(ret, TYPE_STRINGT, machine_name,  config, HK(name));
	hash_data_copy(ret, TYPE_STRINGT, machine_class, config, HK(class));
	
	if(machine_class == NULL || (class = class_find(machine_class)) == NULL)
		return -EINVAL;
	
	if( (machine = machine_find(machine_name)) == NULL ){
		if( (machine = machine_ghost_new(NULL)) == NULL)
			return -ENOMEM;
	}
	
	ret = machine_ghost_resurrect(machine, class, config);
	
	*pmachine = machine;
	return ret;
} // }}}
machine_t *        machine_find(char *name){ // {{{
	machine_t             *machine;
	void                  *list_status  = NULL;
	
	if(name == NULL)
		return NULL;
	
	list_rdlock(&machines_names);
		while( (machine = list_iter_next(&machines_names, &list_status)) != NULL){
			if(machine->name == NULL)
				continue;
			
			if(strcmp(machine->name, name) == 0){
				machine_acquire(machine);
				break;
			}
		}
	list_unlock(&machines_names);
	
	if(!machine)
		machine = machine_ghost_new(name);
	
	return machine;
} // }}}
ssize_t            machine_is_ghost(machine_t *machine){ // {{{
	return (machine->class == NULL) ? 1 : 0;
} // }}}
void               machine_acquire(machine_t *machine){ // {{{
	if(machine)
		machine_ref_inc(machine);
} // }}}
void               machine_destroy(machine_t *machine){ // {{{
	pthread_mutex_lock(&destroy_mtx);
		if(machine_ref_dec(machine) == 0){
			// call destroy
			if(machine->func_destroy != NULL)
				machine->func_destroy(machine);
			
			if(machine->config)
				hash_free(machine->config);

			machine_disconnect(machine->cprev, machine);
			machine_disconnect(machine, machine->cnext);
			
			machine_ghost_free(machine);
		}
	pthread_mutex_unlock(&destroy_mtx);
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

static ssize_t     shop_new_iter(hash_t *item, machine_new_ctx *ctx){ // {{{
	machine_t             *machine;
	
	if( item->key == hash_ptr_null || item->data.type != TYPE_HASHT){
		ctx->machine_next = NULL;
		return ITER_CONTINUE;
	}
	
	if(machine_new(&machine, (hash_t *)(item->data.ptr)) < 0)
		return ITER_BREAK;
	
	machine_connect(machine, ctx->machine_next);
	ctx->machine_next = machine;
	
	list_add(&ctx->machines, machine);
	return ITER_CONTINUE;
} // }}}

machine_t *        shop_new          (hash_t *config){ // {{{
	machine_t             *machine;
	machine_new_ctx        ctx               = { LIST_INITIALIZER, NULL };
	
	if(hash_iter(config, (hash_iterator)&shop_new_iter, &ctx, HASH_ITER_NULL) != ITER_OK){
		while( (machine = list_pop(&ctx.machines)) )
			machine_destroy(machine);
		
		ctx.machine_next = NULL;
	}
	
	list_destroy(&ctx.machines);
	return ctx.machine_next;
} // }}}
void               shop_destroy      (machine_t *machine){ // {{{
	machine_t             *curr;
	machine_t             *next;
	
	for(curr = machine; curr; curr = next){
		next = curr->cnext;
		machine_destroy(curr);
	}
} // }}}

ssize_t            machine_query        (machine_t *machine, request_t *request){ // {{{
	f_crwd                 func              = NULL;
	ssize_t                ret;
	
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
				case ACTION_COUNT:  func = machine->machine_type_crwd.func_count ; break;
				case ACTION_CUSTOM: func = machine->machine_type_crwd.func_custom; break;
				default:
					return -EINVAL;
			};
			break;
		}
	}while(0);
	
	if(func == NULL)
		return machine_query(machine->cnext, request);
	
	pthread_setspecific(key_curr_request, request);
	
	return func(machine, request);
} // }}}
ssize_t            machine_pass         (machine_t *machine, request_t *request){ // {{{
	return machine_query(machine->cnext, request);
} // }}}
ssize_t            machine_fast_query   (machine_t *machine, void *fargs){ // {{{
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
		return request_downgrade(machine, fargs);
	}while(0);
	
	if(func == NULL)
		return machine_fast_pass(machine, fargs);
	
	return func(machine, fargs);
} // }}}
ssize_t            machine_fast_pass    (machine_t *machine, void *fargs){ // {{{
	return machine_fast_query(machine->cnext, fargs);
} // }}}

