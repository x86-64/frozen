#include <libfrozen.h>
#include <regex.h>

/**
 * @ingroup modules
 * @addtogroup mod_backend_regexp Module 'data/regexp'
 */
/**
 * @ingroup mod_backend_regexp
 * @page page_regexp_info Description
 *
 * This module use POSIX regular expressions to redirect flow of request
 */
/**
 * @ingroup mod_backend_regexp
 * @page page_regexp_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/regexp",
 *              regexp                  = "aaa.*",            # regexp for matching
 *              input                   = "url",              # input key for string to match, default "buffer"
 *              reg_extended            = (uint_t)'0',        # 1 - use extended regexp, 0 - basic, default 1
 *              reg_icase               = (uint_t)'0',        # 1 - no case matching, 0 - case matching, default 0
 *              reg_newline             = (uint_t)'0',        # see "man regcomp", default 0
 *              reg_notbol              = (uint_t)'0',        # see "man regcomp", default 0
 *              reg_noteol              = (uint_t)'0',        # see "man regcomp", default 0
 *              pass_to                 =                     # on match - request passed to this backend
 *                                        "index_name",       # existing backend
 *                                        { ... },            # new backend configuration
 * }
 * @endcode
 */
/**
 * @ingroup mod_backend_regexp
 * @page page_regexp_io Input and output
 * 
 * This backend try to match HK( config->input ) with config->regexp. If match found - request redirected to
 * <pass_to> backend. If not - request pass to next backend in chain.
 */

#define EMODULE 27

typedef struct regexp_userdata {
	regex_t                regex;
	uintmax_t              compiled;
	uintmax_t              eflags;
	hash_key_t             hk_input;
	backend_t             *backend;
} regexp_userdata;

static int regexp_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(regexp_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int regexp_destroy(backend_t *backend){ // {{{
	regexp_userdata *userdata = (regexp_userdata *)backend->userdata;
	
	if(userdata->compiled == 1)
		regfree(&userdata->regex);
	
	free(userdata);
	return 0;
} // }}}
static int regexp_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              flag_extended     = 1;
	uintmax_t              flag_icase        = 0;
	uintmax_t              flag_newline      = 0;
	uintmax_t              flag_notbol       = 0;
	uintmax_t              flag_noteol       = 0;
	char                  *regexp_str;
	regexp_userdata       *userdata          = (regexp_userdata *)backend->userdata;
	
	userdata->hk_input = HK(buffer);
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->hk_input,  config, HK(input));
	hash_data_copy(ret, TYPE_UINTT,   flag_extended, config, HK(reg_extended));
	hash_data_copy(ret, TYPE_UINTT,   flag_icase,    config, HK(reg_icase));
	hash_data_copy(ret, TYPE_UINTT,   flag_newline,  config, HK(reg_newline));
	hash_data_copy(ret, TYPE_UINTT,   flag_notbol,   config, HK(reg_notbol));
	hash_data_copy(ret, TYPE_UINTT,   flag_noteol,   config, HK(reg_noteol));
	hash_data_copy(ret, TYPE_STRINGT, regexp_str,    config, HK(regexp));
	if(ret != 0 || regexp_str == NULL)
		return error("HK(regexp) not supplied");
	
	hash_data_copy(ret, TYPE_BACKENDT, userdata->backend, config, HK(pass_to));
	if(ret != 0)
		return error("supplied HK(pass_to) backend not valid, or not found");
	
	if(regcomp(
		&userdata->regex,
		regexp_str,
		((flag_extended != 0) ? REG_EXTENDED : 0) |
		((flag_icase    != 0) ? REG_ICASE    : 0) |
		((flag_newline  != 0) ? REG_NEWLINE  : 0) |
		REG_NOSUB
	) != 0)
		return error("invalid regexp supplied - compilation error");
	
	userdata->compiled   = 1;
	userdata->eflags     = 
		((flag_notbol   != 0) ? REG_NOTBOL   : 0) |
		((flag_noteol   != 0) ? REG_NOTEOL   : 0);
	return 0;
} // }}}

static ssize_t regexp_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	char                  *input_str         = NULL;
	regexp_userdata       *userdata          = (regexp_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, input_str, request, userdata->hk_input);
	if(ret != 0 || input_str == NULL)
		return error("no input string in request");
	
	if(regexec(&userdata->regex, input_str, 0, NULL, userdata->eflags) == 0){
		return backend_query(userdata->backend, request);
	}
	return (ret = backend_pass(backend, request)) < 0 ? ret : -EEXIST;
} // }}}

backend_t regexp_proto = {
	.class          = "data/regexp",
	.supported_api  = API_CRWD,
	.func_init      = &regexp_init,
	.func_configure = &regexp_configure,
	.func_destroy   = &regexp_destroy,
	.backend_type_hash = {
		.func_handler = &regexp_handler
	}
};

