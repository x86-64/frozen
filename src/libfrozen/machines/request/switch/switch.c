#include <libfrozen.h>
#include <shop/pass/pass.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_switch request/switch
 *
 * Switch module can pass requests to different machines according rules for incoming requests
 */
/**
 * @ingroup mod_machine_switch
 * @page page_switch_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class      = "switch",
 * 	        rules      = {
 * 	            { machine = (machine_t)'some1', request = { action = (action_t)'create'                    } },
 * 	            { machine = (machine_t)'some2', request = { action = (action_t)'read', debug = (uint_t)'1' } },
 * 	            { request = { test = (uint_t)'1' } }
 * 	            { machine = (machine_t)'default' },
 * 	        }
 * 	}
 * @endcode
 * This config have 4 rules:
 *  @li call "some1" machine on all ACTION_CREATE requests
 *  @li call "some2" on ACTION_READ requests with debug == '1'
 *  @li pass request to underlying machine then test == '1'
 *  @li call "default" machine on any non matched request
 * Last action is always - pass to underlying machine
 *
 */

#define ERRORS_MODULE_ID         30
#define ERRORS_MODULE_NAME "request/switch"

typedef struct switch_userdata {
	list                   rules;
} switch_userdata;

typedef struct switch_rule {
	hash_t                *request;
	machine_t             *machine;
} switch_rule;

typedef struct switch_context {
	machine_t             *machine;
	request_t             *request;
	ssize_t                ret;
} switch_context;

static ssize_t switch_config_iterator(hash_t *rule_item, machine_t *machine){ // {{{
	ssize_t                ret;
	hash_t                *rule;
	switch_rule           *new_rule;
	switch_userdata       *userdata          = (switch_userdata *)machine->userdata;

	if( (new_rule = calloc(sizeof(switch_rule), 1)) == NULL)
		return ITER_BREAK;
	
	data_get(ret, TYPE_HASHT, rule, &rule_item->data);
	if(ret != 0)
		return ITER_CONTINUE;
	
	hash_data_consume(ret, TYPE_MACHINET, new_rule->machine, rule, HK(machine)); // converted copy of machine
	hash_data_consume(ret, TYPE_HASHT,    new_rule->request, rule, HK(request)); // ptr to config, not copy
	
	list_push(&userdata->rules, new_rule);
	return ITER_CONTINUE;
} // }}}
static ssize_t switch_iterator(switch_rule *rule, switch_context *context){ // {{{
	data_t           d_rule_request = DATA_PTR_HASHT(rule->request);
	data_t           d_ctx_request  = DATA_PTR_HASHT(context->request);
	fastcall_compare r_compare = { { 3, ACTION_COMPARE }, &d_ctx_request };
	
	if(data_query(&d_rule_request, &r_compare) == 0 && r_compare.result == 0)
		goto query;
	
	return 1;

query:
	if(rule->machine == NULL)
		goto pass;
	
	stack_call(context->machine);
		
		context->ret = machine_query(rule->machine, context->request);
		
	stack_clean();
	return 0;
pass:
	context->ret = machine_pass(context->machine, context->request);
	return 0;
} // }}}

static ssize_t switch_init(machine_t *machine){ // {{{
	if( (machine->userdata = calloc(1, sizeof(switch_userdata))) == NULL)
		return error("calloc returns null");
	
	list_init(& ((switch_userdata *)machine->userdata)->rules);
	return 0;
} // }}}
static ssize_t switch_destroy(machine_t *machine){ // {{{
	switch_userdata       *userdata          = (switch_userdata *)machine->userdata;
	switch_rule           *rule;
	
	while( (rule = list_pop(&userdata->rules)) != NULL){
		pipeline_destroy(rule->machine);
		hash_free(rule->request);
		free(rule);
	}
	list_destroy(&userdata->rules);

	free(userdata);
	return 0;
} // }}}
static ssize_t switch_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	hash_t                *rules             = NULL;
	
	hash_data_get(ret, TYPE_HASHT, rules, config, HK(rules));
	
	if(hash_iter(rules, (hash_iterator)&switch_config_iterator, machine, 0) != ITER_OK)
		return error("failed to configure switch");
	
	return 0;
} // }}}

static ssize_t switch_handler(machine_t *machine, request_t *request){ // {{{
	void                  *iter              = NULL;
	switch_rule           *rule;
	switch_context         context           = { machine->cnext, request };
	switch_userdata       *userdata          = (switch_userdata *)machine->userdata;
	
	request_enter_context(request);

		while( (rule = list_iter_next(&userdata->rules, &iter)) != NULL){
			if(switch_iterator(rule, &context) == 0){
				request_leave_context();
				return context.ret;
			}
		}
	
	request_leave_context();
	return machine_pass(machine, request);
} // }}}

machine_t switch_proto = {
	.class          = "request/switch",
	.supported_api  = API_HASH,
	.func_init      = &switch_init,
	.func_configure = &switch_configure,
	.func_destroy   = &switch_destroy,
	.machine_type_hash = {
		.func_handler = &switch_handler
	}
};

