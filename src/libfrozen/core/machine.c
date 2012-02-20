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

static machine_t machine_dummy = { // {{{
	.machine_type_hash = {
		.func_handler = &api_machine_nosys
	}
}; // }}}

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
void               request_set_current(request_t *request){ // {{{
	pthread_setspecific(key_curr_request, request);
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
		parent->cnext = child ? child : &machine_dummy;
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
		parent->cnext = &machine_dummy;
	}
	if(child){
		machine_top(child);
		child->cprev  = NULL;
	}
} // }}}
static ssize_t     machine_dummy_pass(machine_t *machine, request_t *request){ // {{{
	return machine_pass(machine, request);
} // }}}
static void        machine_fill_blanks(machine_t *machine){ // {{{
	if( (machine->supported_api & API_HASH) != 0){
		if(machine->machine_type_hash.func_handler == NULL)
			machine->machine_type_hash.func_handler = &machine_dummy_pass;
	}
	if(machine->cnext == NULL){
		machine->cnext = &machine_dummy;
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
	ssize_t                ret;
	char                  *name = machine->name;
	uintmax_t              refs = machine->refs;
	
	memcpy(machine, class, sizeof(machine_t)); // TODO remove this
	machine->name = name;
	machine->refs = refs;
	
	if(machine->func_init != NULL && (ret = machine->func_init(machine)) < 0)
		return ret;
	
	if(machine->func_configure != NULL && (ret = machine->func_configure(machine, config)) < 0)
		goto error;
	
	machine_fill_blanks(machine);
	return 0;
	
error:
	if(machine->func_destroy != NULL)
		machine->func_destroy(machine);
	
	return ret;
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
	
	hash_data_convert(ret, TYPE_STRINGT, machine_name,  config, HK(name));
	hash_data_convert(ret, TYPE_STRINGT, machine_class, config, HK(class));
	
	if(machine_class == NULL || (class = class_find(machine_class)) == NULL){
		ret = -EINVAL;
		goto exit;
	}
	
	if( (machine = machine_find(machine_name)) == NULL ){
		if( (machine = machine_ghost_new(NULL)) == NULL){
			ret = -ENOMEM;
			goto exit;
		}
	}
	
	ret = machine_ghost_resurrect(machine, class, config);
	
	*pmachine = machine;

exit:
	if(machine_name)
		free(machine_name);
	if(machine_class)
		free(machine_class);
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
	ssize_t                ret;
	machine_t             *machine;
	hash_t                *config;
	
	if(item->key == hash_ptr_null)
		goto error;
	
	data_consume(ret, TYPE_HASHT, config, (&(item->data)));
	if(ret != 0)
		goto error;
	
	if( (ret = machine_new(&machine, config)) < 0){
		log_error("shop_new error: %d: %s\n", ret, describe_error(ret));
		
		hash_free(config);
		return ITER_BREAK;
	}
	
	machine_connect(machine, ctx->machine_next);
	ctx->machine_next = machine;
	
	list_add(&ctx->machines, machine);
	hash_free(config);
	return ITER_CONTINUE;

error:
	ctx->machine_next = NULL;
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
	
	for(curr = machine; curr != &machine_dummy; curr = next){
		next = curr->cnext;
		machine_destroy(curr);
	}
} // }}}
/*
ssize_t            machine_query        (machine_t *machine, request_t *request){ // {{{
	return machine->machine_type_hash.func_handler(machine, request);
} // }}}
ssize_t            machine_pass         (machine_t *machine, request_t *request){ // {{{
	return machine->cnext->machine_type_hash.func_handler(machine->cnext, request);
} // }}}
*/
