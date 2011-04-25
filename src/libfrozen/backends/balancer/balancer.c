#include <libfrozen.h>
#include <alloca.h>

/**
 * @balancer balancer.c
 * @ingroup modules
 * @brief Balancer module
 *
 * Balancer module can pass requests to different backends according defined rules
 */
/**
 * @ingroup modules
 * @addtogroup mod_balancer Module 'balancer'
 */
/**
 * @ingroup mod_balancer
 * @page page_balancer_config Balancer configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class      = "balancer",
 * 	        mode       =               # balancer mode:
 * 	                     "random",     #   - pass request to random backend, this is default
 * 	                     "counting",   #   - first request go to first backend, second to second, and so on
 * 	            TODO     "hash",       #   - hash <field> in request and pass. Note, no rebuilding on pool change
 * 	                     "linear"      #   - get first <linear_len> bytes and pass to backend forked with same bytes
 *              pool       =               # pool creation:
 *                           "manual",     #   - i'll setup pool manually, this is default
 *                           "fork",       #   - fork each child on moment of creation <pool_size> times
 *                           "childs",     #   - use existing childs as current pool
 *              pool_size  = (size_t)"1",  # number of backends to fork, default 1
 *              field      = "keyname",    # request field to parse in data mode
 *              linear_len = (size_t)'1',  # length of bytes in linear mode. This use sizeof(void *)*(256 ^ <linear_len>) bytes of memory min.
 *              clone      = (size_t)"1",  # clone request and pass to <number> of backends using current mode,
 * 	}
 * @endcode
 */

#define EMODULE         14
#define MAX_LINEAR_LEN  sizeof(size_t)

typedef enum balancer_mode {
	MODE_RANDOM,
	MODE_COUNTING,
	MODE_HASH,
	MODE_LINEAR,
	MODE_DEFAULT = MODE_RANDOM
} balancer_mode;

typedef enum balancer_pool {
	POOL_MANUAL,
	POOL_FORK,
	POOL_CHILDS,
	POOL_DEFAULT = POOL_MANUAL
} balancer_pool;

typedef struct balancer_userdata {
	balancer_mode          mode;
	hash_t                *fork_req;
	hash_key_t             field;
	size_t                 counter;
	size_t                 clone;
	size_t                 linear_len;
	backend_t            **linear_pool;
	list                  *pool;
	list                   own_pool;
} balancer_userdata;

static ssize_t balancer_request_rest   (backend_t *backend, request_t *request);
static ssize_t balancer_request_linear (backend_t *backend, request_t *request);

static balancer_mode    balancer_string_to_mode(char *string){ // {{{
	if(string != NULL){
		if(strcmp(string, "random")   == 0) return MODE_RANDOM;
		if(strcmp(string, "counting") == 0) return MODE_COUNTING;
		if(strcmp(string, "hash")     == 0) return MODE_HASH;
		if(strcmp(string, "linear")   == 0) return MODE_LINEAR;
	}
	return MODE_DEFAULT;
} // }}}
static balancer_pool    balancer_string_to_pool(char *string){ // {{{
	if(string != NULL){
		if(strcmp(string, "manual")   == 0) return POOL_MANUAL;
		if(strcmp(string, "fork")     == 0) return POOL_FORK;
		if(strcmp(string, "childs")   == 0) return POOL_CHILDS;
	}
	return POOL_DEFAULT;
} // }}}

static backend_t * linear_get_backend(backend_t *backend, char *array){ // {{{
	size_t                 array_val;
	backend_t             *curr              = NULL;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	
	array_val = *((size_t *)array);
	
	if( (curr = userdata->linear_pool[array_val]) == NULL){
		uintmax_t    lchilds_size;
		backend_t  **lchilds;
		
		list_rdlock(&backend->childs);
			
			if( (lchilds_size = list_count(&backend->childs)) != 0){
				lchilds = alloca(sizeof(backend_t *) * lchilds_size);
				list_flatten(&backend->childs, (void **)lchilds, lchilds_size);
			}
			
		list_unlock(&backend->childs);
		
		if(lchilds_size != 1)
			goto exit;
		
		request_t r_fork[] = {
			{ userdata->field, DATA_STRING(array) },
			hash_next(userdata->fork_req)
		};
		
		if( (curr = backend_fork(lchilds[0], r_fork)) == NULL)
			goto exit;
		
		userdata->linear_pool[array_val] = curr;
	}
exit:

	return curr;
} // }}}
static void    linear_remove_backend(backend_t *backend, char *array){ // {{{
	size_t                 array_val;
	backend_t             *curr;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	
	array_val = *((size_t *)array);
	
	if( (curr = userdata->linear_pool[array_val]) != NULL){
		backend_destroy(curr);

		userdata->linear_pool[array_val] = NULL;
	}

} // }}}

static int balancer_init(backend_t *backend){ // {{{
	if( (backend->userdata = calloc(1, sizeof(balancer_userdata))) == NULL)
		return error("calloc returns null");
	
	return 0;
} // }}}
static int balancer_destroy(backend_t *backend){ // {{{
	backend_t             *curr;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	
	if(userdata->mode == MODE_LINEAR){
		if(userdata->linear_pool){
			uintmax_t i, linear_len;
			
			safe_pow(&linear_len, 256, userdata->linear_len);
			for(i=0; i<linear_len; i++){
				if( userdata->linear_pool[i] != NULL)
					backend_destroy( userdata->linear_pool[i] );
			}
			
			free(userdata->linear_pool);
		}
	}
	
	if(userdata->pool == &userdata->own_pool){
		while( (curr = list_pop(&userdata->own_pool)) )
			backend_destroy(curr);
	}
	list_destroy(&userdata->own_pool);
	hash_free(userdata->fork_req);
	
	free(userdata);
	return 0;
} // }}}
static int balancer_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              t;
	hash_t                 cfg_fork_def[]    = { hash_end };
	hash_t                *cfg_fork_req      = cfg_fork_def;
	char                  *cfg_mode          = NULL;
	char                  *cfg_pool          = NULL;
	char                  *cfg_field         = NULL;
	size_t                 cfg_clone         = 1;
	size_t                 cfg_pool_size     = 1;
	size_t                 cfg_linear_len    = 1;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_mode,       config, HK(mode));
	hash_data_copy(ret, TYPE_STRINGT, cfg_pool,       config, HK(pool));
	hash_data_copy(ret, TYPE_STRINGT, cfg_field,      config, HK(field));
	hash_data_copy(ret, TYPE_SIZET,   cfg_clone,      config, HK(clone));
	hash_data_copy(ret, TYPE_SIZET,   cfg_pool_size,  config, HK(pool_size));
	hash_data_copy(ret, TYPE_SIZET,   cfg_linear_len, config, HK(linear_len));
	hash_data_copy(ret, TYPE_HASHT,   cfg_fork_req,   config, HK(fork_request));
	
	if(
		cfg_linear_len > MAX_LINEAR_LEN           ||
		safe_pow(&t, 256, cfg_linear_len) < 0     ||
		safe_mul(&t, t, sizeof(backend_t *)) < 0
	)
		return error("invalid linear_len supplied");
	
	userdata->mode       = balancer_string_to_mode(cfg_mode);
	userdata->field      = hash_string_to_key(cfg_field);
	userdata->fork_req   = hash_copy(cfg_fork_req);
	userdata->counter    = 0;
	userdata->clone      = cfg_clone;
	userdata->linear_len = cfg_linear_len;
	list_init(&userdata->own_pool);
	
	switch(balancer_string_to_pool(cfg_pool)){
		case POOL_FORK:;
			size_t       i, j;
			uintmax_t    lchilds_size;
			backend_t  **lchilds;
			backend_t   *curr;
			
			list_rdlock(&backend->childs);
				
				if( (lchilds_size = list_count(&backend->childs)) != 0){
					lchilds = alloca(sizeof(backend_t *) * lchilds_size);
					list_flatten(&backend->childs, (void **)lchilds, lchilds_size);
				}
				
			list_unlock(&backend->childs);
			
			for(i=0; i<cfg_pool_size; i++){
				for(j=0; j<lchilds_size; j++){
					if( (curr = backend_fork(lchilds[j], userdata->fork_req)) == NULL)
						return error("fork failed");
					
					list_add(&userdata->own_pool, curr);
				}
			}
			
			/* fall */
		case POOL_MANUAL:
		default:
			userdata->pool = &userdata->own_pool;
			break;
		case POOL_CHILDS:
			userdata->pool = &backend->childs;
			break;
	};
	
	switch(userdata->mode){
		case MODE_LINEAR:
			if( (userdata->linear_pool = calloc(1, t)) == NULL)
				return error("malloc failed");
			
			backend->backend_type_crwd.func_create = &balancer_request_linear;
			backend->backend_type_crwd.func_set    = &balancer_request_linear;
			backend->backend_type_crwd.func_get    = &balancer_request_linear;
			backend->backend_type_crwd.func_delete = &balancer_request_linear;
			backend->backend_type_crwd.func_move   = &balancer_request_linear;
			backend->backend_type_crwd.func_count  = &balancer_request_linear;
			break;
		
		default:
			backend->backend_type_crwd.func_create = &balancer_request_rest;
			backend->backend_type_crwd.func_set    = &balancer_request_rest;
			backend->backend_type_crwd.func_get    = &balancer_request_rest;
			backend->backend_type_crwd.func_delete = &balancer_request_rest;
			backend->backend_type_crwd.func_move   = &balancer_request_rest;
			backend->backend_type_crwd.func_count  = &balancer_request_rest;
			break;
	};
	
	return 0;
} // }}}

static ssize_t balancer_request_rest(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              lsz, i;
	list                  *pool;
	backend_t            **lchilds;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	size_t                 clone_i           = userdata->clone;
	
	pool = userdata->pool;
	
	list_rdlock(pool);
		
		if( (lsz = list_count(pool)) != 0){
			lchilds = alloca( sizeof(backend_t *) * lsz );
			list_flatten(pool, (void **)lchilds, lsz);
		}
		
	list_unlock(pool);
	
	if(lsz == 0)
		return error("no childs");
	
redo:
	switch(userdata->mode){
		case MODE_RANDOM:
			i = random();
			break;
		case MODE_COUNTING:
			i = userdata->counter++;
			break;
		default:
			i = 0;
			break;
	};
	i %= lsz;
	if( (ret = backend_query(lchilds[i], request)) < 0)
		return ret;
	
	if(--clone_i > 0) goto redo;
	
	return -EEXIST;
} // }}}
static ssize_t balancer_request_linear(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	char                   array[MAX_LINEAR_LEN + 1] = {0};
	backend_t             *curr;
	data_t                *field;
	data_ctx_t            *field_ctx;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	size_t                 clone_i           = userdata->clone;
	
	hash_data_find(request, userdata->field, &field, &field_ctx);
	if(field == NULL)
		return error("field not supplied");
	
	if(data_read(field, field_ctx, 0, &array, userdata->linear_len) < 0)
		return error("data_read failed");
	
	if( (curr = linear_get_backend(backend, array)) == NULL)
		return error("fork error");
	
redo:
	if( (ret = backend_query(curr, request)) < 0){
		if(ret == -EBADF){ // backend dead
			linear_remove_backend(backend, array); // remove corpse
			
			if( (curr = linear_get_backend(backend, array)) == NULL)
				return error("fork error (resume)");
			
			goto redo;
		}
		return ret;
	}
	
	if(--clone_i > 0) goto redo;
	
	return -EEXIST;
} // }}}

backend_t balancer_proto = {
	.class          = "balancer",
	.supported_api  = API_CRWD,
	.func_init      = &balancer_init,
	.func_configure = &balancer_configure,
	.func_destroy   = &balancer_destroy
};

