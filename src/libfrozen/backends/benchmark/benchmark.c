#include <libfrozen.h>
#include <strings.h>
#include <pthread.h>

#define EMODULE 17

typedef enum bench_mode {
	MODE_ACTIVE,
	MODE_PASSIVE,

	MODE_DEFAULT = MODE_ACTIVE
} bench_mode;

typedef struct benchmark_userdata {
	bench_mode             mode;

	hash_t                *prereq;
	hash_t                *req;
	hash_t                *postreq;
	
	pthread_t              bench_thread;
	
	uintmax_t              ticks;
	struct timeval         tv_start;
} benchmark_userdata;

static ssize_t benchmark_handler_passive(backend_t *backend, request_t *request);

/* benchmarks {{{ */
static void timeval_subtract (result, x, y)
        struct timeval *result, *x, *y; 
{
    
        if (x->tv_usec < y->tv_usec) {
                int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
                y->tv_usec -= 1000000 * nsec;
                y->tv_sec += nsec;
        }   
        if (x->tv_usec - y->tv_usec > 1000000) {
                int nsec = (x->tv_usec - y->tv_usec) / 1000000;
                y->tv_usec += 1000000 * nsec;
                y->tv_sec -= nsec;
        }   
        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_usec = x->tv_usec - y->tv_usec;
}
/* }}} */

static bench_mode  benchmark_mode_from_string(char *string){ // {{{
	if(string != NULL){
		if(strcasecmp(string, "active")  == 0) return MODE_ACTIVE;
		if(strcasecmp(string, "passive") == 0) return MODE_PASSIVE;
	}
	return MODE_DEFAULT;
} // }}}
static void        benchmark_set_handler(backend_t *backend, f_crwd func){ // {{{
	backend->backend_type_crwd.func_create = func;
	backend->backend_type_crwd.func_set    = func;
	backend->backend_type_crwd.func_get    = func;
	backend->backend_type_crwd.func_delete = func;
	backend->backend_type_crwd.func_move   = func;
	backend->backend_type_crwd.func_count  = func;
} // }}}

void *  benchmark_thread  (void *backend){ // {{{
	//benchmark_userdata    *userdata          = (benchmark_userdata *)((backend_t *)backend)->userdata;
	
	return NULL;
} // }}}

static void benchmark_control_restart(backend_t *backend){ // {{{
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	userdata->ticks = 0;
	gettimeofday(&userdata->tv_start, NULL);
} // }}}
static int  benchmark_control_query_short(backend_t *backend, char *string, size_t size){ // {{{
        struct timeval         tv_end, tv_diff;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	gettimeofday(&tv_end,   NULL);
	
	timeval_subtract(&tv_diff, &tv_end, &userdata->tv_start);
        
	return snprintf(string, size, "%3u.%.6u", (unsigned int)tv_diff.tv_sec, (unsigned int)tv_diff.tv_usec);
} // }}}
static int  benchmark_control_query_long(backend_t *backend, char *string, size_t size){ // {{{
        char                  *backend_name;
	uintmax_t              ticks, speed = 0, one = 0;
	struct timeval         tv_end, tv_diff;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	gettimeofday(&tv_end,   NULL);
	ticks = userdata->ticks;
	
	timeval_subtract(&tv_diff, &tv_end, &userdata->tv_start);
	
	backend_name = backend_get_name(backend);
	backend_name = backend_name ? backend_name : "(undefined)";
	if(ticks != 0){
		// TODO rewrite math
		speed  = ( ticks * 1000 / ( tv_diff.tv_usec / 1000 + tv_diff.tv_sec * 1000 ) );
		one    = ( (tv_diff.tv_usec         + tv_diff.tv_sec * 1000000) / ticks );
	}
	
	return snprintf(string, size,
		"benchmark '%s': %7lu ticks in %3lu.%.6lu sec; %7lu ticks/s; one: %10lu us",
		backend_name,
		(unsigned long)ticks,
		(unsigned long)tv_diff.tv_sec, (unsigned long)tv_diff.tv_usec,
		(unsigned long)speed,
		(unsigned long)one
	);
} // }}}
static void benchmark_control_query_ms(backend_t *backend, uintmax_t *value){ // {{{
	struct timeval         tv_end, tv_diff;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	gettimeofday(&tv_end,   NULL);
	
	timeval_subtract(&tv_diff, &tv_end, &userdata->tv_start);
        
	*value = tv_diff.tv_usec / 1000  + tv_diff.tv_sec * 1000;
	if(*value == 0) *value = 1;
} // }}}
static void benchmark_control_query_us(backend_t *backend, uintmax_t *value){ // {{{
	struct timeval         tv_end, tv_diff;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	gettimeofday(&tv_end,   NULL);
	
	timeval_subtract(&tv_diff, &tv_end, &userdata->tv_start);
        
	*value = tv_diff.tv_usec         + tv_diff.tv_sec * 1000000;
	if(*value == 0) *value = 1;
} // }}}
static void benchmark_control_query_ticks(backend_t *backend, uintmax_t *value){ // {{{
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	*value = userdata->ticks;
} // }}}

static int benchmark_init(backend_t *backend){ // {{{
	benchmark_userdata    *userdata          = backend->userdata = calloc(1, sizeof(benchmark_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	return 0;
} // }}}
static int benchmark_destroy(backend_t *backend){ // {{{
	void                  *res;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	if(userdata->mode == MODE_ACTIVE){
		// TODO error handling
		pthread_cancel(userdata->bench_thread);
		pthread_join(userdata->bench_thread, &res);
		
		if(userdata->prereq)
			hash_free(userdata->prereq);
		if(userdata->req)
			hash_free(userdata->req);
		if(userdata->postreq)
			hash_free(userdata->postreq);
	}
	
	free(userdata);
	return 0;
} // }}}
static int benchmark_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	char                  *cfg_mode          = NULL;
	hash_t                 req_default[]     = { hash_end };
	hash_t                *cfg_prereq        = req_default;
	hash_t                *cfg_req           = req_default;
	hash_t                *cfg_postreq       = req_default;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_mode,       config, HK(mode));
	hash_data_copy(ret, TYPE_HASHT,   cfg_prereq,     config, HK(pre_request));
	hash_data_copy(ret, TYPE_HASHT,   cfg_req,        config, HK(request));
	hash_data_copy(ret, TYPE_HASHT,   cfg_postreq,    config, HK(post_request));
	
	switch( (userdata->mode = benchmark_mode_from_string(cfg_mode)) ){
		case MODE_ACTIVE:
			userdata->prereq  = hash_copy(cfg_prereq);
			userdata->req     = hash_copy(cfg_req);
			userdata->postreq = hash_copy(cfg_postreq);
			
			if(pthread_create(&userdata->bench_thread, NULL, &benchmark_thread, backend) != 0)
				return error("pthread_create failed");
			
			break;
		case MODE_PASSIVE:
			benchmark_set_handler(backend, &benchmark_handler_passive);
			break;
	};
	
	return 0;
} // }}}

static ssize_t benchmark_handler_passive(backend_t *backend, request_t *request){ // {{{
	return -EINVAL;
} // }}}
static ssize_t benchmark_handler_custom(backend_t *backend, request_t *request_t){ // {{{
	(void)benchmark_control_restart;
	(void)benchmark_control_query_short;
	(void)benchmark_control_query_long;
	(void)benchmark_control_query_ticks;
	(void)benchmark_control_query_ms;
	(void)benchmark_control_query_us;
	return 0;
} // }}}

backend_t benchmark_proto = {
	.class          = "benchmark",
	.supported_api  = API_CRWD,
	.func_init      = &benchmark_init,
	.func_configure = &benchmark_configure,
	.func_destroy   = &benchmark_destroy,
	{
		.func_custom = &benchmark_handler_custom
	}
};


