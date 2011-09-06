#include <libfrozen.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#define EMODULE 17
#define N_REQUESTS_DEFAULT 10000

typedef enum bench_action {
	ACTION_NONE,
	ACTION_PRINTF,
} bench_action;

typedef struct benchmark_userdata {
	bench_action           on_destroy;
	
	uintmax_t              ticks;
	struct timeval         tv_start;
} benchmark_userdata;

static bench_action    benchmark_action_from_string(char *string){ // {{{
	if(string != NULL){
		if(strcasecmp(string, "printf")  == 0) return ACTION_PRINTF;
	}
	return ACTION_NONE;
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
	
	timersub(&tv_end, &userdata->tv_start, &tv_diff);
        
	return snprintf(string, size, "%3u.%.6u", (unsigned int)tv_diff.tv_sec, (unsigned int)tv_diff.tv_usec);
} // }}}
static int  benchmark_control_query_long(backend_t *backend, char *string, size_t size){ // {{{
        char                  *backend_name;
	uintmax_t              ticks, speed = 0, one = 0;
	struct timeval         tv_end, tv_diff;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	gettimeofday(&tv_end,   NULL);
	ticks = userdata->ticks;
	
	timersub(&tv_end, &userdata->tv_start, &tv_diff);
	
	backend_name = backend->name;
	backend_name = backend_name ? backend_name : "(undefined)";
	if(ticks != 0){
		// TODO rewrite math
		speed  = tv_diff.tv_usec / 1000 + tv_diff.tv_sec * 1000;
		speed  = (speed != 0) ? speed : 1;
		speed  = ticks * 1000 / speed;
		one    = ( (tv_diff.tv_usec         + tv_diff.tv_sec * 1000000) / ticks );
	}
	
	return snprintf(string, size,
		"benchmark '%10s': %7lu ticks in %3lu.%.6lu sec; %7lu ticks/s; one: %10lu us",
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
	
	timersub(&tv_end, &userdata->tv_start, &tv_diff);
        
	*value = tv_diff.tv_usec / 1000  + tv_diff.tv_sec * 1000;
	if(*value == 0) *value = 1;
} // }}}
static void benchmark_control_query_us(backend_t *backend, uintmax_t *value){ // {{{
	struct timeval         tv_end, tv_diff;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	gettimeofday(&tv_end,   NULL);
	
	timersub(&tv_end, &userdata->tv_start, &tv_diff);
        
	*value = tv_diff.tv_usec         + tv_diff.tv_sec * 1000000;
	if(*value == 0) *value = 1;
} // }}}
static void benchmark_control_query_ticks(backend_t *backend, uintmax_t *value){ // {{{
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	*value = userdata->ticks;
} // }}}
static void benchmark_action(backend_t *backend, bench_action action){ // {{{
	switch(action){
		case ACTION_NONE:
			break;
		case ACTION_PRINTF:;
			char buffer[1024];
			
			if(benchmark_control_query_long(backend, buffer, sizeof(buffer)) > 0){
				printf("%s\n", buffer);
			}
			break;
	};
} // }}}

static int benchmark_init(backend_t *backend){ // {{{
	benchmark_userdata    *userdata          = backend->userdata = calloc(1, sizeof(benchmark_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	return 0;
} // }}}
static int benchmark_destroy(backend_t *backend){ // {{{
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	benchmark_action(backend, userdata->on_destroy);
	
	free(userdata);
	return 0;
} // }}}
static int benchmark_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	char                  *cfg_ondestroy     = NULL;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_ondestroy,  config, HK(on_destroy)); (void)ret;
	
	userdata->on_destroy = benchmark_action_from_string(cfg_ondestroy);
	if(userdata->on_destroy == ACTION_NONE) userdata->on_destroy = ACTION_PRINTF;
	
	benchmark_control_restart(backend);
	
	return 0;
} // }}}

static ssize_t benchmark_handler_passive(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	userdata->ticks++;
	
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
} // }}}
static ssize_t benchmark_handler_custom(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	char                  *function;
	data_t                *value;
	data_t                *string;
	
	hash_data_copy(ret, TYPE_STRINGT,  function,  request, HK(function));
	if(ret != 0)
		goto pass;
	
	hash_data_find(request, HK(value),  &value,  NULL);
	hash_data_find(request, HK(string), &string, NULL);
	
	if(strcmp(function, "benchmark_restart") == 0){
		benchmark_control_restart(backend);
		return 0;
	}
	
	if(string != NULL){
		if(strcmp(function, "benchmark_short") == 0)
			return benchmark_control_query_short (backend, data_value_ptr(string), data_value_len(string) );
		if(strcmp(function, "benchmark_long") == 0)
			return benchmark_control_query_long  (backend, data_value_ptr(string), data_value_len(string) );
	}
	
	if(value != NULL && data_value_type(value) == TYPE_UINTT){
		if(strcmp(function, "benchmark_ticks") == 0){
			benchmark_control_query_ticks(backend, data_value_ptr(value) );
			return 0;
		}
		if(strcmp(function, "benchmark_ms") == 0){
			benchmark_control_query_ms(backend, data_value_ptr(value) );
			return 0;
		}
		if(strcmp(function, "benchmark_us") == 0){
			benchmark_control_query_us(backend, data_value_ptr(value) );
			return 0;
		}
	}
pass:
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
} // }}}

backend_t benchmark_proto = {
	.class          = "backend/benchmark",
	.supported_api  = API_CRWD,
	.func_init      = &benchmark_init,
	.func_configure = &benchmark_configure,
	.func_destroy   = &benchmark_destroy,
	{
		.func_create = &benchmark_handler_passive,
		.func_get    = &benchmark_handler_passive,
		.func_set    = &benchmark_handler_passive,
		.func_delete = &benchmark_handler_passive,
		.func_move   = &benchmark_handler_passive,
		.func_count  = &benchmark_handler_passive,
		.func_custom = &benchmark_handler_custom
	}
};

