#include <libfrozen.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_benchmark backend/benchmark
 */
/**
 * @ingroup mod_backend_benchmark
 * @page page_benchmark_info Description
 *
 * This backend count requests and measure time from backend creation to backend destory. Useful in pair with emitter and it's "destroy"
 * option.
 */
/**
 * @ingroup mod_backend_benchmark
 * @page page_benchmark_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "backend/benchmark"
 * }
 * @endcode
 */
/**
 * @ingroup mod_backend_benchmark
 * @page page_benchmark_control Control
 * 
 * Any request with HK(benchmark_function) used to control this backend. Possible requests:
 * @code
 * {
 *              benchmark_function               =
 *                                       "restart",   # restart clocks and counter
 *                                       "short",     # write short stats to buffer HK(string)
 *                                       "long",      # write long stats to buffer HK(string)
 *                                       "ticks",     # write number of requests passed by to HK(value)
 *                                       "us",        # write number of nanoseconds passed from restart to HK(value)
 *                                       "ms",        # write number of miliseconds passed from restart to HK(value)
 *
 * }
 * @endcode
 */

#define EMODULE 17
#define N_REQUESTS_DEFAULT 10000

typedef struct benchmark_userdata {
	uintmax_t              ticks;
	struct timeval         tv_start;
} benchmark_userdata;

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

static int benchmark_init(backend_t *backend){ // {{{
	benchmark_userdata    *userdata          = backend->userdata = calloc(1, sizeof(benchmark_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	return 0;
} // }}}
static int benchmark_destroy(backend_t *backend){ // {{{
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}

static ssize_t benchmark_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	char                  *function;
	data_t                *value;
	data_t                *string;
	char                   buffer[1024];
	benchmark_userdata    *userdata          = (benchmark_userdata *)backend->userdata;
	
	userdata->ticks++;
	
	hash_data_copy(ret, TYPE_STRINGT,  function,  request, HK(benchmark_function));
	if(ret == 0)
		goto custom;
	
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
custom:	
	value  = hash_data_find(request, HK(value));
	string = hash_data_find(request, HK(string));
	
	if(strcmp(function, "restart") == 0){
		benchmark_control_restart(backend);
		return 0;
	}
	if(strcmp(function, "print_long") == 0){
		ret = benchmark_control_query_long  (backend, buffer, sizeof(buffer) );
		printf("%s\n", buffer);
		return ret;
	}
	
	if(string != NULL){
		if(strcmp(function, "short") == 0){
			ret = benchmark_control_query_short (backend, buffer, sizeof(buffer) );
			goto write;
		}
		if(strcmp(function, "long") == 0){
			ret = benchmark_control_query_long  (backend, buffer, sizeof(buffer) );
			goto write;
		}
	}
	
	if(value != NULL && value->type == TYPE_UINTT){
		if(strcmp(function, "ticks") == 0){
			benchmark_control_query_ticks(backend, value->ptr );
			return 0;
		}
		if(strcmp(function, "ms") == 0){
			benchmark_control_query_ms(backend, value->ptr );
			return 0;
		}
		if(strcmp(function, "us") == 0){
			benchmark_control_query_us(backend, value->ptr );
			return 0;
		}
	}
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
write:;
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &buffer, sizeof(buffer) };	
	data_query(string, &r_write);
	return ret;
} // }}}

backend_t benchmark_proto = {
	.class          = "backend/benchmark",
	.supported_api  = API_HASH,
	.func_init      = &benchmark_init,
	.func_destroy   = &benchmark_destroy,
	.backend_type_hash = {
		.func_handler = &benchmark_handler
	}
};

