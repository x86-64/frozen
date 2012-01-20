#include <libfrozen.h>
#include <machine.h>
#include <pthread.h>

/**
 * @ingroup machine
 * @addtogroup mod_pool machine/pool
 *
 * Pool module inserted in forkable machine shop can track any property of machine (like memory usage)
 * and limit it. Underlying modules can be destroyed or call special function.
 *
 */
/**
 * @ingroup mod_pool
 * @page page_pool_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class             = "pool",
 * 	        parameter         =                    # Step 1: how <parameter> value will be obtained:
 * 	                            "ticks",           #  - <parameter> is ticks collected in last <ticks_interval> seconds, default
 * 	                            "request",         #  - send <parameter_request> + HK(buffer) -> (uint_t).
 * 	                            "one",             #  - <parameter> == 1 if pool not killed
 *
 *                                                     # Step 2: limit
 *              max_perinstance   = (uint_t)'1000',    # set maximum <parameter> value for one instance to 1000
 *              max_perfork       = (uint_t)'2000',    # set maximum <parameter> value for group of forked machines to 2000
 *              max_global        = (uint_t)'3000',    # set maximum <parameter> value for all machines (can be different for each instance)
 *
 *              mode_perfork      =                    # Step 3: choose victum
 *                                  "first",           #   - earliest registered machine, default
 *                                  "last",            #   - latest registered machine
 *                                  "random",          #   - random item
 *                                  "highest",         #   - machine with higest <parameter> value
 *                                  "lowest",          #   - machine with lowest <parameter> value
 *              mode_global       ... same as _perfork #
 *                                                     
 *                                                     # Step 4: what to do with victum?
 *              action            =                    # what to do with choosed module:
 *                                  "request"          #   - send request <action_request_one>, default
 *              action_one        ... same as action   # override for _one action
 *              action_perfork    ... same as action   # override for _perfork action
 *              action_global     ... same as action   # override for _global action
 *              
 *              parameter_request      = { ... }       #
 *              action_request         = { ... }       # this is default action
 *              action_request_one     = { ... }       # override for _one limits
 *              action_request_perfork = { ... }       # override for _perfork limits
 *              action_request_global  = { ... }       # override for _global limits
 *              
 *              pool_interval     = (uint_t)'5'        # see below. This parameter is global,
 *                                                       newly created machines overwrite it
 *              tick_interval     = (uint_t)'5'        # see below.
 * 	}
 *
 * Limit behavior:
 *   - Limit checked ones per <pool_interval> (default 5 seconds). Between two checks machines can overconsume limit, but this method
 *     will have less performance loss.
 *   - Ticks collected in <tick_interval> period. One tick is one passed request.
 *
 * @endcode
 */

#define EMODULE 15
#define POOL_INTERVAL_DEFAULT 5
#define TICK_INTERVAL_DEFAULT 5

typedef enum parameter_method {
	PARAMETER_REQUEST,
	PARAMETER_ONE,
	PARAMETER_TICKS,
	
	PARAMETER_DEFAULT = PARAMETER_TICKS
} parameter_method;

typedef enum choose_method {
	METHOD_RANDOM,
	METHOD_FIRST,
	METHOD_LAST,
	METHOD_HIGHEST,
	METHOD_LOWEST,
	
	METHOD_DEFAULT = METHOD_FIRST
} choose_method;

typedef enum action_method {
	ACTION_DESTROY,
	ACTION_REQUEST,
	
	ACTION_DEFAULT = ACTION_REQUEST
} action_method;

typedef struct rule {
	uintmax_t        limit;
	choose_method    mode;
	action_method    action;
	request_t       *action_request;
} rule;

typedef struct pool_userdata {
	// usage params
	uintmax_t        created_on;
	uintmax_t        usage_parameter;
	uintmax_t        usage_ticks_last;  // 
	uintmax_t        usage_ticks_curr;  // no locks for this, coz we don't care about precise info
	uintmax_t        usage_ticks_time;  //
	
	// fork support
	list            *perfork_childs;
	list             perfork_childs_own;
	
	parameter_method parameter;
	hash_t          *parameter_request;
	uintmax_t        tick_interval;
	
	rule             rule_one;
	rule             rule_perfork;
	rule             rule_global;
} pool_userdata;

static uintmax_t        pool_interval          = POOL_INTERVAL_DEFAULT;
static uintmax_t        running_pools          = 0;
static pthread_mutex_t  running_pools_mtx      = PTHREAD_MUTEX_INITIALIZER;
static pthread_t        main_thread;
static list             watch_one;
static list             watch_perfork;
static list             watch_global;

static ssize_t pool_machine_request_cticks(machine_t *machine, request_t *request);
static ssize_t pool_machine_request_died(machine_t *machine, request_t *request);

static parameter_method  pool_string_to_parameter(char *string){ // {{{
	if(string != NULL){
		if(strcasecmp(string, "request") == 0)        return PARAMETER_REQUEST;
		if(strcasecmp(string, "ticks") == 0)          return PARAMETER_TICKS;
		if(strcasecmp(string, "one") == 0)            return PARAMETER_ONE;
	}
	return PARAMETER_DEFAULT;
} // }}}
static choose_method     pool_string_to_method(char *string){ // {{{
	if(string != NULL){
		if(strcasecmp(string, "random") == 0)        return METHOD_RANDOM;
		if(strcasecmp(string, "first") == 0)         return METHOD_FIRST;
		if(strcasecmp(string, "last") == 0)          return METHOD_LAST;
		if(strcasecmp(string, "highest") == 0)       return METHOD_HIGHEST;
		if(strcasecmp(string, "lowest") == 0)        return METHOD_LOWEST;
	}
	return METHOD_DEFAULT;
} // }}}
static action_method     pool_string_to_action(char *string){ // {{{
	if(string != NULL){
		if(strcasecmp(string, "destroy") == 0) return ACTION_DESTROY;
		if(strcasecmp(string, "request") == 0) return ACTION_REQUEST;
	}
	return ACTION_DEFAULT;
} // }}}

static void pool_set_handler(machine_t *machine, f_crwd func){ // {{{
	machine->machine_type_crwd.func_create = func;
	machine->machine_type_crwd.func_set    = func;
	machine->machine_type_crwd.func_get    = func;
	machine->machine_type_crwd.func_delete = func;
	machine->machine_type_crwd.func_move   = func;
	machine->machine_type_crwd.func_count  = func;
} // }}}
uintmax_t   pool_parameter_get(machine_t *machine, pool_userdata *userdata){ // {{{
	switch(userdata->parameter){
		case PARAMETER_ONE:   return (machine->machine_type_crwd.func_create == &pool_machine_request_died) ? 0 : 1;
		case PARAMETER_TICKS: return userdata->usage_ticks_last;
		case PARAMETER_REQUEST:;
			uintmax_t buffer    = 0;
			request_t r_query[] = {
				{ HK(buffer), DATA_PTR_UINTT(&buffer) },
				hash_next(userdata->parameter_request)
			};
			machine_pass(machine, r_query);
			
			return buffer;
		default:
			break;
	};
	return 0;
} // }}}
void        pool_machine_action(machine_t *machine, rule *rule){ // {{{
	pool_userdata        *userdata          = (pool_userdata *)machine->userdata;
	
	switch(rule->action){
		case ACTION_DESTROY:;
			// set machine mode to died
			pool_set_handler(machine, &pool_machine_request_died);
			
			if(userdata->perfork_childs != &userdata->perfork_childs_own){
				// kill childs for forked shops, keep for initial
				shop_destroy(machine->cnext);
			}
			
			break;
		case ACTION_REQUEST:;
			machine_pass(machine, rule->action_request);
			break;
		default:
			break;
	};
	
	// update parameter
	userdata->usage_parameter = pool_parameter_get(machine, userdata);
} // }}}
void        pool_group_limit(list *machines, rule *rule, uintmax_t need_lock){ // {{{
	uintmax_t              i, lsz;
	uintmax_t              parameter;
	uintmax_t              parameter_group;
	machine_t             *machine;
	pool_userdata         *machine_ud;
	machine_t            **lchilds;
	
	if(rule->limit == __MAX(uintmax_t)) // no limits
		return;
	
	if(need_lock == 1) list_rdlock(machines);
		
		if( (lsz = list_count(machines)) != 0){
			lchilds = alloca( sizeof(machine_t *) * lsz );
			list_flatten(machines, (void **)lchilds, lsz);
			
repeat:;
			parameter_group = 0;
			
			machine_t *machine_first         = NULL;
			machine_t *machine_last          = NULL;
			machine_t *machine_high          = NULL;
			machine_t *machine_low           = NULL;
			
			for(i=0; i<lsz; i++){
				machine    = lchilds[i];
				machine_ud = (pool_userdata *)machine->userdata;
				
				if( (parameter = machine_ud->usage_parameter) == 0)
					continue;
				
				parameter_group += parameter;
				
				#define pool_kill_check(_machine, _field, _moreless) { \
					if(_machine == NULL){ \
						_machine = machine; \
					}else{ \
						if(machine_ud->_field _moreless ((pool_userdata *)_machine->userdata)->_field) \
							_machine = machine; \
					} \
				}
				pool_kill_check(machine_first,      created_on,       <);
				pool_kill_check(machine_last,       created_on,       >);
				pool_kill_check(machine_low,        usage_parameter,  <);
				pool_kill_check(machine_high,       usage_parameter,  >);
			}
			
			if(parameter_group >= rule->limit){
				switch(rule->mode){
					case METHOD_RANDOM:         machine = lchilds[ random() % lsz ]; break;
					case METHOD_FIRST:          machine = machine_first; break;
					case METHOD_LAST:           machine = machine_last; break;
					case METHOD_HIGHEST:        machine = machine_high; break;
					case METHOD_LOWEST:         machine = machine_low; break;
					default:                    machine = NULL; break;
				};
				if(machine != NULL){
					pool_machine_action(machine, rule);
					goto repeat;
				}
			}
		}
		
	if(need_lock == 1) list_unlock(machines);
	
	return;
} // }}}
void *      pool_main_thread(void *null){ // {{{
	void                  *iter;
	machine_t             *machine;
	pool_userdata         *machine_ud;
	
	while(1){
		// update parameter + limit one
		list_rdlock(&watch_one);
			iter = NULL;
			while( (machine = list_iter_next(&watch_one, &iter)) != NULL){
				machine_ud = (pool_userdata *)machine->userdata;
				
				machine_ud->usage_parameter = pool_parameter_get(machine, machine_ud);
				
				if(machine_ud->usage_parameter > machine_ud->rule_one.limit)
					pool_machine_action(machine, &machine_ud->rule_one);
			}
		list_unlock(&watch_one);
		
		// preform perfork limits
		list_rdlock(&watch_perfork);
			iter = NULL;
			while( (machine = list_iter_next(&watch_perfork, &iter)) != NULL){
				machine_ud = (pool_userdata *)machine->userdata;
				
				pool_group_limit(machine_ud->perfork_childs, &machine_ud->rule_perfork, 1);
			}
		list_unlock(&watch_perfork);
		
		// preform global limits
		list_rdlock(&watch_global);
			iter = NULL;
			while( (machine = list_iter_next(&watch_global, &iter)) != NULL){
				machine_ud = (pool_userdata *)machine->userdata;
				
				pool_group_limit(&watch_global, &machine_ud->rule_global, 0);
			}
		list_unlock(&watch_global);
		
		sleep(pool_interval);
	};
	return NULL;
} // }}}

static int pool_init(machine_t *machine){ // {{{
	ssize_t                ret               = 0;
	pool_userdata         *userdata          = machine->userdata = calloc(1, sizeof(pool_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	// global
	pthread_mutex_lock(&running_pools_mtx);
		if(running_pools++ == 0){
			list_init(&watch_one);
			list_init(&watch_perfork);
			list_init(&watch_global);
			
			if(pthread_create(&main_thread, NULL, &pool_main_thread, NULL) != 0)
				ret = error("pthread_create failed");
		}
	pthread_mutex_unlock(&running_pools_mtx);
	
	return ret;
} // }}}
static int pool_destroy(machine_t *machine){ // {{{
	void                  *res;
	ssize_t                ret               = 0;
	pool_userdata         *userdata          = (pool_userdata *)machine->userdata;
	
	if(userdata->perfork_childs == &userdata->perfork_childs_own){
		list_destroy(&userdata->perfork_childs_own);
	}else{
		list_delete(userdata->perfork_childs, machine);
	}
	hash_free(userdata->parameter_request);
	hash_free(userdata->rule_one.action_request);
	hash_free(userdata->rule_perfork.action_request);
	hash_free(userdata->rule_global.action_request);
	list_delete(&watch_one,     machine);
	list_delete(&watch_perfork, machine);
	list_delete(&watch_global,  machine);
	free(userdata);
	
	// global
	pthread_mutex_lock(&running_pools_mtx);
		if(--running_pools == 0){
			if(pthread_cancel(main_thread) != 0){
				ret = error("pthread_cancel failed");
				goto exit;
			}
			if(pthread_join(main_thread, &res) != 0){
				ret = error("pthread_join failed");
				goto exit;
			}
			// TODO memleak here
			
			list_destroy(&watch_one);
			list_destroy(&watch_perfork);
			list_destroy(&watch_global);
		}
exit:
	pthread_mutex_unlock(&running_pools_mtx);
	
	return ret;
} // }}}
static int pool_configure_any(machine_t *machine, machine_t *parent, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              cfg_limit_one     = __MAX(uintmax_t);
	uintmax_t              cfg_limit_fork    = __MAX(uintmax_t);
	uintmax_t              cfg_limit_global  = __MAX(uintmax_t);
	uintmax_t              cfg_tick          = TICK_INTERVAL_DEFAULT;
	uintmax_t              cfg_pool          = POOL_INTERVAL_DEFAULT;
	char                  *cfg_parameter     = NULL;
	char                  *cfg_act           = NULL;
	char                  *cfg_act_one       = NULL;
	char                  *cfg_act_perfork   = NULL;
	char                  *cfg_act_global    = NULL;
	char                  *cfg_mode_perfork  = NULL;
	char                  *cfg_mode_global   = NULL;
	hash_t                 cfg_act_req_def[] = { hash_end };
	hash_t                *cfg_act_req       = cfg_act_req_def;
	hash_t                *cfg_act_req_one;
	hash_t                *cfg_act_req_frk;
	hash_t                *cfg_act_req_glb;
	hash_t                *cfg_param_req     = cfg_act_req_def;
	pool_userdata         *userdata          = (pool_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_STRINGT, cfg_parameter,    config, HK(parameter));
	hash_data_get(ret, TYPE_STRINGT, cfg_mode_perfork, config, HK(mode_perfork));
	hash_data_get(ret, TYPE_STRINGT, cfg_mode_global,  config, HK(mode_global));
	hash_data_get(ret, TYPE_UINTT,   cfg_tick,         config, HK(tick_interval));
	
	// actions
	hash_data_get(ret, TYPE_STRINGT, cfg_act,          config, HK(action));
	cfg_act_one = cfg_act;
	cfg_act_perfork = cfg_act;
	cfg_act_global = cfg_act;
	hash_data_get(ret, TYPE_STRINGT, cfg_act_one,      config, HK(action_one));
	hash_data_get(ret, TYPE_STRINGT, cfg_act_perfork,  config, HK(action_perfork));
	hash_data_get(ret, TYPE_STRINGT, cfg_act_global,   config, HK(action_global));
	
	// requests
	hash_data_consume(ret, TYPE_HASHT,   cfg_param_req,    config, HK(parameter_request));
	
	hash_data_consume(ret, TYPE_HASHT,   cfg_act_req,      config, HK(action_request));
	cfg_act_req_one = cfg_act_req;
	cfg_act_req_frk = cfg_act_req;
	cfg_act_req_glb = cfg_act_req;
	hash_data_consume(ret, TYPE_HASHT,   cfg_act_req_one,  config, HK(action_request_one));
	hash_data_consume(ret, TYPE_HASHT,   cfg_act_req_frk,  config, HK(action_request_perfork));
	hash_data_consume(ret, TYPE_HASHT,   cfg_act_req_glb,  config, HK(action_request_global));
	
	hash_data_get(ret, TYPE_UINTT,   cfg_pool,         config, HK(pool_interval));
	if(ret != 0)
		pool_interval = cfg_pool;
	
	userdata->parameter                   = pool_string_to_parameter(cfg_parameter);
	userdata->parameter_request           = cfg_param_req ? cfg_param_req : hash_copy(cfg_act_req_def);
	userdata->rule_perfork.mode           = pool_string_to_method(cfg_mode_perfork);
	userdata->rule_global.mode            = pool_string_to_method(cfg_mode_global);
	userdata->rule_one.action             = pool_string_to_action(cfg_act_perfork);
	userdata->rule_perfork.action         = pool_string_to_action(cfg_act_perfork);
	userdata->rule_global.action          = pool_string_to_action(cfg_act_global);
	userdata->rule_one.action_request     = cfg_act_req_one;
	userdata->rule_perfork.action_request = cfg_act_req_frk;
	userdata->rule_global.action_request  = cfg_act_req_glb;
	userdata->tick_interval               = cfg_tick;
	userdata->created_on                  = time(NULL);
	
	hash_data_get(ret, TYPE_UINTT,   cfg_limit_one,    config, HK(max_perinstance));
	userdata->rule_one.limit              = cfg_limit_one;
	list_add(&watch_one, machine);
	
	hash_data_get(ret, TYPE_UINTT,   cfg_limit_fork,   config, HK(max_perfork));
	userdata->rule_perfork.limit          = cfg_limit_fork;
	if(ret == 0)
		list_add(&watch_perfork, (parent == NULL) ? machine : parent);
	
	hash_data_get(ret, TYPE_UINTT,   cfg_limit_global, config, HK(max_global));
	userdata->rule_global.limit           = cfg_limit_global;
	if(ret == 0)
		list_add(&watch_global,  machine);
	
	if(userdata->parameter == PARAMETER_TICKS){
		pool_set_handler(machine, &pool_machine_request_cticks);
	}else{
		pool_set_handler(machine, NULL);
	}
	
	return 0;
} // }}}
static int pool_configure(machine_t *machine, config_t *config){ // {{{
	pool_userdata        *userdata          = (pool_userdata *)machine->userdata;
	
	// no fork, so use own data
	userdata->perfork_childs       = &userdata->perfork_childs_own;
	list_init(&userdata->perfork_childs_own);
	
	return pool_configure_any(machine, NULL, config);
} // }}}

static ssize_t pool_machine_request_cticks(machine_t *machine, request_t *request){ // {{{
	uintmax_t              time_curr;
	pool_userdata         *userdata = (pool_userdata *)machine->userdata;
	
	// update usage ticks
	
	time_curr = time(NULL);
	if(userdata->usage_ticks_time + userdata->tick_interval < time_curr){
		userdata->usage_ticks_time = time_curr;
		userdata->usage_ticks_last = userdata->usage_ticks_curr;
		userdata->usage_ticks_curr = 0;
	}
	userdata->usage_ticks_curr++;
	
	return machine_pass(machine, request);
} // }}}
static ssize_t pool_machine_request_died(machine_t *machine, request_t *request){ // {{{
	return -EBADF;
} // }}}

machine_t pool_proto = {
	.class          = "machine/pool",
	.supported_api  = API_CRWD,
	.func_init      = &pool_init,
	.func_configure = &pool_configure,
	.func_destroy   = &pool_destroy
};

