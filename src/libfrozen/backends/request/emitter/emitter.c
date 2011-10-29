#include <libfrozen.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_emitter Backend 'request/emitter'
 */
/**
 * @ingroup mod_backend_emitter
 * @page page_emitter_info Description
 *
 * This backend send user defined queries in any amounts
 */
/**
 * @ingroup mod_backend_emitter
 * @page page_emitter_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/emitter",
 *              on_start                = <see actions>,      # action performed at the beginning of emission
 *              on_end                  = <see actions>,      # action performed at the end of emission
 *              pre_request             = { ... },            # optional request emitted at the beginning of emission
 *              post_request            = { ... },            # optional request emitted at the end of emission
 *              request                 = { ... },            # actual request, that would be emitted
 *              count                   = (uint_t)'1000',     # number of emits
 * }
 * @endcode
 *
 * Possible actions:
 *   @li "destroy" - full shutdown of application via kill(SIGTERM)
 */

#define EMODULE 18
#define N_REQUESTS_DEFAULT __MAX(uintmax_t)

typedef enum emitter_action {
	ACTION_NONE,
	ACTION_DESTROY
} emitter_action;

typedef struct emitter_userdata {
	emitter_action         on_start;
	emitter_action         on_end;
	
	hash_t                *prereq;
	hash_t                *req;
	hash_t                *postreq;
	uintmax_t              nreq;
	
	uintmax_t              emitter_started;
	pthread_t              emitter_thread;
} emitter_userdata;

static emitter_action    emitter_action_from_string(char *string){ // {{{
	if(string != NULL){
		if(strcasecmp(string, "destroy") == 0) return ACTION_DESTROY;
	}
	return ACTION_NONE;
} // }}}
static void              emitter_do_action(backend_t *backend, emitter_action action){ // {{{
	switch(action){
		case ACTION_NONE:
			break;
		case ACTION_DESTROY:
			kill(getpid(), SIGTERM);
			break;
	};
} // }}}
static void *            emitter_thread  (void *backend){ // {{{
	uintmax_t            i;
	emitter_userdata    *userdata          = (emitter_userdata *)((backend_t *)backend)->userdata;
	
	emitter_do_action(backend, userdata->on_start);
	if(userdata->prereq)
		backend_pass(backend, userdata->prereq);
	
	if(userdata->req){
		for(i = 0; i < userdata->nreq; i++)
			backend_pass(backend, userdata->req);
	}
	
	if(userdata->postreq)
		backend_pass(backend, userdata->postreq);
	
	emitter_do_action(backend, userdata->on_end);
	return NULL;
} // }}}

static int emitter_init(backend_t *backend){ // {{{
	emitter_userdata    *userdata          = backend->userdata = calloc(1, sizeof(emitter_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->nreq = N_REQUESTS_DEFAULT;
	return 0;
} // }}}
static int emitter_destroy(backend_t *backend){ // {{{
	void                  *res;
	emitter_userdata      *userdata          = (emitter_userdata *)backend->userdata;
	
	if(userdata->emitter_started != 0){
		pthread_cancel(userdata->emitter_thread);
		pthread_join(userdata->emitter_thread, &res);
	}
	
	free(userdata);
	return 0;
} // }}}
static int emitter_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	char                  *cfg_on_start;
	char                  *cfg_on_end;
	emitter_userdata      *userdata          = (emitter_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT,   userdata->prereq,     config, HK(pre_request));
	hash_data_copy(ret, TYPE_HASHT,   userdata->req,        config, HK(request));
	hash_data_copy(ret, TYPE_HASHT,   userdata->postreq,    config, HK(post_request));
	hash_data_copy(ret, TYPE_UINTT,   userdata->nreq,       config, HK(count));
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_on_start,         config, HK(on_start));
	if(ret == 0)
		userdata->on_start = emitter_action_from_string(cfg_on_start);
		
	hash_data_copy(ret, TYPE_STRINGT, cfg_on_end,           config, HK(on_end));
	if(ret == 0)
		userdata->on_end   = emitter_action_from_string(cfg_on_end);
			
	if(userdata->emitter_started == 0){
		if(pthread_create(&userdata->emitter_thread, NULL, &emitter_thread, backend) != 0)
			return error("pthread_create failed");
		
		userdata->emitter_started = 1;
	}
	return 0;
} // }}}

backend_t emitter_proto = {
	.class          = "request/emitter",
	.supported_api  = API_HASH,
	.func_init      = &emitter_init,
	.func_configure = &emitter_configure,
	.func_destroy   = &emitter_destroy,
};

