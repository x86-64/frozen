#include <libfrozen.h>

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
 * 	        class     = "balancer",
 * 	        mode      =               # balancer mode:
 * 	                    "random",     #   - pass request to random backend
 * 	                    "counting",   #   - first request go to first backend, second to second, and so on
 * 	                    "data",       #   - watch data in request and make decision using it
 *              pool      =               # pool creation:
 *                          "manual",     #   - i'll setup pool manually
 *                          "fork",       #   - fork each child on moment of creation <pool_size> times
 *                          "childs",     #   - use existing childs as current pool
 *              pool_size = (size_t)"1",  # number of backends to fork
 *              clone     = (size_t)"1",  # clone request and pass to <number> of backends using current mode,
 * 	}
 * @endcode
 */

#define EMODULE         14

typedef enum balancer_mode {
	MODE_RANDOM,
	MODE_COUNTING,
	MODE_DATA,
	MODE_DEFAULT = MODE_RANDOM
} balancer_mode;

typedef enum balancer_pool {
	POOL_MANUAL,
	POOL_FORK,
	POOL_CHILDS,
	POOL_DEFAULT = POOL_FORK
} balancer_pool;

typedef struct balancer_userdata {
	balancer_mode          mode;
	size_t                 counter;
	size_t                 clone;
	list                  *pool;
	list                   own_pool;
} balancer_userdata;

static balancer_mode    balancer_string_to_mode(char *string){ // {{{
	if(string != NULL){
		if(strcmp(string, "random")   == 0) return MODE_RANDOM;
		if(strcmp(string, "counting") == 0) return MODE_COUNTING;
		if(strcmp(string, "data")     == 0) return MODE_DATA;
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

static int balancer_init(backend_t *backend){ // {{{
	if( (backend->userdata = calloc(1, sizeof(balancer_userdata))) == NULL)
		return error("calloc returns null");
	
	return 0;
} // }}}
static int balancer_destroy(backend_t *backend){ // {{{
	backend_t             *curr;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	
	if(userdata->pool == &userdata->own_pool){
		while( (curr = list_pop(&userdata->own_pool)) )
			backend_destroy(curr);
	}
	list_destroy(&userdata->own_pool);
	
	free(userdata);
	return 0;
} // }}}
static int balancer_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	hash_t                 cfg_fork_def[]    = { hash_end };
	hash_t                *cfg_fork_req      = cfg_fork_def;
	char                  *cfg_mode          = NULL;
	char                  *cfg_pool          = NULL;
	size_t                 cfg_clone         = 1;
	size_t                 cfg_pool_size     = 1;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_mode,      config, HK(mode));
	hash_data_copy(ret, TYPE_STRINGT, cfg_pool,      config, HK(pool));
	hash_data_copy(ret, TYPE_SIZET,   cfg_clone,     config, HK(clone));
	hash_data_copy(ret, TYPE_SIZET,   cfg_pool_size, config, HK(pool_size));
	hash_data_copy(ret, TYPE_HASHT,   cfg_fork_req,  config, HK(fork_request));
	
	userdata->mode    = balancer_string_to_mode(cfg_mode);
	userdata->counter = 0;
	userdata->clone   = cfg_clone;
	list_init(&userdata->own_pool);
	
	switch(balancer_string_to_pool(cfg_pool)){
		case POOL_FORK:;
			size_t       i, j;
			uintmax_t    lchilds_size;
			backend_t  **lchilds;
			backend_t   *curr;
			
			if( (lchilds_size = list_flatten_frst(&backend->childs)) != 0){
				if( (lchilds = malloc(sizeof(backend_t *) * lchilds_size)) == NULL)
					return error("no memory");
				list_flatten_scnd(&backend->childs, (void **)lchilds, lchilds_size);
				
				for(i=0; i<cfg_pool_size; i++){
					for(j=0; j<lchilds_size; j++){
						if( (curr = backend_fork(lchilds[j], cfg_fork_req)) == NULL)
							return error("fork failed");
						
						list_add(&userdata->own_pool, curr);
					}
				}
				free(lchilds);
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
	
	return 0;
} // }}}

static ssize_t balancer_request(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              lsz, i;
	list                  *pool;
	backend_t            **lchilds;
	balancer_userdata     *userdata          = (balancer_userdata *)backend->userdata;
	
	pool = userdata->pool;
	if( (lsz = list_flatten_frst(pool)) == 0)
		return error("no childs");
	
	lchilds = alloca( sizeof(backend_t *) * lsz );
	list_flatten_scnd(pool, (void **)lchilds, lsz);
	
	switch(userdata->mode){
		case MODE_RANDOM:
			i = random();
			break;
		case MODE_COUNTING:
			i = userdata->counter++;
			break;
		case MODE_DATA:
			i = 0;
			break;
	};
	i %= lsz;
	if( (ret = backend_query(lchilds[i], request)) < 0)
		return ret;
	
	return -EEXIST;
} // }}}

backend_t balancer_proto = {
	.class          = "balancer",
	.supported_api  = API_CRWD,
	.func_init      = &balancer_init,
	.func_configure = &balancer_configure,
	.func_destroy   = &balancer_destroy,
	{{
		.func_create = &balancer_request,
		.func_set    = &balancer_request,
		.func_get    = &balancer_request,
		.func_delete = &balancer_request,
		.func_move   = &balancer_request,
		.func_count  = &balancer_request
	}}
};


