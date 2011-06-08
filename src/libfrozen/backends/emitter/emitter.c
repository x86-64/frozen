#include <libfrozen.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

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
	
	for(i = 0; i < userdata->nreq; i++)
		backend_pass(backend, userdata->req);
	
	if(userdata->postreq)
		backend_pass(backend, userdata->postreq);
	
	emitter_do_action(backend, userdata->on_end);
	return NULL;
} // }}}

static int emitter_init(backend_t *backend){ // {{{
	emitter_userdata    *userdata          = backend->userdata = calloc(1, sizeof(emitter_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	return 0;
} // }}}
static int emitter_destroy(backend_t *backend){ // {{{
	void                  *res;
	emitter_userdata      *userdata          = (emitter_userdata *)backend->userdata;
	
	pthread_cancel(userdata->emitter_thread);
	pthread_join(userdata->emitter_thread, &res);
	
	if(userdata->prereq)
		hash_free(userdata->prereq);
	if(userdata->req)
		hash_free(userdata->req);
	if(userdata->postreq)
		hash_free(userdata->postreq);
	
	free(userdata);
	return 0;
} // }}}
static int emitter_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	char                  *cfg_on_start      = NULL;
	char                  *cfg_on_end        = NULL;
	hash_t                 req_default[]     = { { HK(action), DATA_UINT32T(ACTION_CRWD_CREATE) }, hash_end };
	hash_t                *cfg_prereq        = NULL;
	hash_t                *cfg_req           = req_default;
	hash_t                *cfg_postreq       = NULL;
	uintmax_t              cfg_nreq          = N_REQUESTS_DEFAULT;
	emitter_userdata      *userdata          = (emitter_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_on_start,   config, HK(on_start));
	hash_data_copy(ret, TYPE_STRINGT, cfg_on_end,     config, HK(on_end));
	hash_data_copy(ret, TYPE_HASHT,   cfg_prereq,     config, HK(pre_request));
	hash_data_copy(ret, TYPE_HASHT,   cfg_req,        config, HK(request));
	hash_data_copy(ret, TYPE_HASHT,   cfg_postreq,    config, HK(post_request));
	hash_data_copy(ret, TYPE_UINTT,   cfg_nreq,       config, HK(count));
	
	userdata->on_start = emitter_action_from_string(cfg_on_start);
	userdata->on_end   = emitter_action_from_string(cfg_on_end);
	userdata->prereq   = cfg_prereq  ? hash_copy(cfg_prereq)  : NULL;
	userdata->req      = hash_copy(cfg_req);
	userdata->postreq  = cfg_postreq ? hash_copy(cfg_postreq) : NULL;
	userdata->nreq     = cfg_nreq;
			
	if(pthread_create(&userdata->emitter_thread, NULL, &emitter_thread, backend) != 0)
		return error("pthread_create failed");
			
	return 0;
} // }}}

backend_t emitter_proto = {
	.class          = "emitter",
	.supported_api  = API_CRWD,
	.func_init      = &emitter_init,
	.func_configure = &emitter_configure,
	.func_destroy   = &emitter_destroy,
};

