#include <libfrozen.h>

/**
 * @switch switch.c
 * @ingroup modules
 * @brief Switch module
 *
 * Switch module can pass requests to different backends according rules for incoming requests
 */
/**
 * @ingroup modules
 * @addtogroup mod_switch Module 'switch'
 */
/**
 * @ingroup mod_switch
 * @page page_switch_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class      = "switch",
 * 	        rules      = {
 * 	            { backend = (backend_t)'some1', request = { action = (action_t)'create'                    } },
 * 	            { backend = (backend_t)'some2', request = { action = (action_t)'read', debug = (uint_t)'1' } },
 * 	            { request = { test = (uint_t)'1' } }
 * 	            { backend = (backend_t)'default' },
 * 	        }
 * 	}
 * @endcode
 * This config have 4 rules:
 *  @li call "some1" backend on all ACTION_CREATE requests
 *  @li call "some2" on ACTION_READ requests with debug == '1'
 *  @li pass request to underlying backend then test == '1'
 *  @li call "default" backend on any non matched request
 * Last action is always - pass to underlying backend
 *
 */

#define EMODULE         30

typedef struct switch_userdata {
	list                   rules;
} switch_userdata;

typedef struct switch_rule {
	hash_t                *request;
	backend_t             *backend;
} switch_rule;

typedef struct switch_context {
	backend_t             *backend;
	request_t             *request;
	ssize_t                ret;
} switch_context;

static ssize_t switch_config_iterator(hash_t *rule_item, list *childs, switch_userdata *userdata){ // {{{
	ssize_t                ret;
	hash_t                *rule;
	switch_rule           *new_rule;
	
	if( (new_rule = calloc(sizeof(switch_rule), 1)) == NULL)
		return ITER_BREAK;
	
	data_get(ret, TYPE_HASHT, rule, &rule_item->data);
	if(ret != 0)
		return ITER_CONTINUE;
	
	hash_data_copy(ret, TYPE_BACKENDT, new_rule->backend, rule, HK(backend)); // converted copy of backend
	hash_data_copy(ret, TYPE_HASHT,    new_rule->request, rule, HK(request)); // ptr to config, not copy
	
	if(new_rule->backend != NULL)
		backend_add_terminators(new_rule->backend, childs);
	
	list_add(&userdata->rules, new_rule);
	return ITER_CONTINUE;
} // }}}
static ssize_t switch_iterator(switch_rule *rule, switch_context *context){ // {{{
	ssize_t                ret;
	
	data_t           d_rule_request = DATA_PTR_HASHT(rule->request);
	data_t           d_ctx_request  = DATA_PTR_HASHT(context->request);
	fastcall_compare r_compare = { { 3, ACTION_COMPARE }, &d_ctx_request };
	
	if(data_query(&d_rule_request, &r_compare) == 0)
		goto query;
	
	return 1;

query:
	if(rule->backend == NULL)
		goto pass;
	
	context->ret = backend_query(rule->backend, context->request);
	return 0;
pass:
	context->ret = ( ret = backend_pass(context->backend, context->request) ) ? ret : -EEXIST;
	return 0;
} // }}}

static int switch_init(backend_t *backend){ // {{{
	if( (backend->userdata = calloc(1, sizeof(switch_userdata))) == NULL)
		return error("calloc returns null");
	
	list_init(& ((switch_userdata *)backend->userdata)->rules);
	return 0;
} // }}}
static int switch_destroy(backend_t *backend){ // {{{
	switch_userdata       *userdata          = (switch_userdata *)backend->userdata;
	switch_rule           *rule;
	
	while( (rule = list_pop(&userdata->rules)) != NULL){
		if(rule->backend)
			backend_destroy(rule->backend);
		free(rule);
	}
	list_destroy(&userdata->rules);

	free(userdata);
	return 0;
} // }}}
static int switch_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	hash_t                *rules;
	switch_userdata       *userdata          = (switch_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT, rules, config, HK(rules));
	
	if(hash_iter(rules, (hash_iterator)&switch_config_iterator, &backend->childs, userdata) != ITER_OK)
		return error("failed to configure switch");
	
	return 0;
} // }}}

static ssize_t switch_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	void                  *childs_iter       = NULL;
	switch_rule           *rule;
	switch_context         context           = { backend, request };
	switch_userdata       *userdata          = (switch_userdata *)backend->userdata;
	
	while( (rule = list_iter_next(&userdata->rules, &childs_iter)) != NULL){
		if(switch_iterator(rule, &context) == 0)
			return context.ret;
	}
	
	return ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
} // }}}

backend_t switch_proto = {
	.class          = "request/switch",
	.supported_api  = API_HASH,
	.func_init      = &switch_init,
	.func_configure = &switch_configure,
	.func_destroy   = &switch_destroy,
	.backend_type_hash = {
		.func_handler = &switch_handler
	}
};

