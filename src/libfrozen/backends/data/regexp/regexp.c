#include <libfrozen.h>
#include <regex.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_regexp Backend 'data/regexp'
 */
/**
 * @ingroup mod_backend_regexp
 * @page page_regexp_info Description
 *
 * This module use POSIX regular expressions to match data.
 *
 * Currently only TYPE_STRINGT as input supported
 */
/**
 * @ingroup mod_backend_regexp
 * @page page_regexp_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/regexp",
 *              regexp                  = "aaa.*",            # regexp for matching, default ".*"
 *              input                   = "url",              # input key for string to match, default "buffer"
 *              reg_extended            = (uint_t)'0',        # 1 - use extended regexp, 0 - basic, default 0
 *              reg_icase               = (uint_t)'0',        # 1 - no case matching, 0 - case matching, default 0
 *              reg_newline             = (uint_t)'0',        # see "man regcomp", default 0
 *              reg_notbol              = (uint_t)'0',        # see "man regcomp", default 0
 *              reg_noteol              = (uint_t)'0',        # see "man regcomp", default 0
 *              marker                  = (hashkey_t)'marker',# on match - pass request with this key set
 *              marker_value            = (uint_t)'1',        # value for marker
 * }
 * @endcode
 */

#define EMODULE 27

typedef struct regexp_userdata {
	char                  *regexp_str;
	uintmax_t              cflags;
	uintmax_t              eflags;
	hash_key_t             input;
	hash_key_t             marker;
	data_t                *marker_data;
	
	uintmax_t              compiled;
	regex_t                regex;
} regexp_userdata;

data_t                         marker_default    = DATA_UINTT(1);

static void    config_updateflag(hash_t *config, hash_key_t key, uintmax_t value, uintmax_t *flag){ // {{{
	ssize_t                ret;
	uintmax_t              new_value;
	
	hash_data_copy(ret, TYPE_UINTT, new_value, config, key);
	if(ret == 0){
		if(new_value == 0){
			*flag &= ~value;
		}else{
			*flag |= value;
		}
	}
} // }}}

static ssize_t config_newregexp(regexp_userdata *userdata){ // {{{
	if(regcomp(&userdata->regex, userdata->regexp_str, userdata->cflags) != 0)
		return error("invalid regexp supplied - compilation error");
	
	userdata->compiled = 1;
	return 0;
} // }}}
static void    config_freeregexp(regexp_userdata *userdata){ // {{{
	if(userdata->compiled == 1)
		regfree(&userdata->regex);
} // }}}

static ssize_t config_newmarkerdata(regexp_userdata *userdata, data_t *marker_data){ // {{{
	data_t                *new_data;
	
	if( (new_data = malloc(sizeof(data_t))) == NULL)
		return -ENOMEM;
	
	fastcall_copy r_copy = { { 3, ACTION_COPY }, new_data };
	if(data_query(marker_data, &r_copy) != 0){
		free(new_data);
		return error("can not copy marker data");
	}
	
	userdata->marker_data = new_data;
	return 0;
} // }}}
static void    config_freemarkerdata(regexp_userdata *userdata){ // {{{
	if(userdata->marker_data != &marker_default){
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(userdata->marker_data, &r_free);
		
		free(userdata->marker_data);
	}
} // }}}

static int regexp_init(backend_t *backend){ // {{{
	regexp_userdata       *userdata;

	if((userdata = backend->userdata = calloc(1, sizeof(regexp_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->input       = HK(buffer);
	userdata->marker      = HK(marker);
	userdata->marker_data = &marker_default;
	userdata->cflags      = REG_NOSUB;
	userdata->regexp_str  = strdup(".*");
	return 0;
} // }}}
static int regexp_destroy(backend_t *backend){ // {{{
	regexp_userdata       *userdata          = (regexp_userdata *)backend->userdata;
	
	config_freeregexp(userdata);
	config_freemarkerdata(userdata);
	
	free(userdata);
	return 0;
} // }}}
static int regexp_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	data_t                *marker_data;
	char                  *regexp_str;
	regexp_userdata       *userdata          = (regexp_userdata *)backend->userdata;
	
	config_updateflag(config, HK(reg_extended), REG_EXTENDED, &userdata->cflags);
	config_updateflag(config, HK(reg_icase),    REG_ICASE,    &userdata->cflags);
	config_updateflag(config, HK(reg_newline),  REG_NEWLINE,  &userdata->cflags);
	
	config_updateflag(config, HK(reg_notbol),   REG_NOTBOL,   &userdata->eflags);
	config_updateflag(config, HK(reg_noteol),   REG_NOTEOL,   &userdata->eflags);
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,  config, HK(input));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->marker, config, HK(marker));
	
	hash_data_copy(ret, TYPE_STRINGT,  regexp_str,       config, HK(regexp));
	if(ret == 0){
		free(userdata->regexp_str);
		userdata->regexp_str = strdup(regexp_str);
	}
	
	config_freeregexp(userdata);
	if( (ret = config_newregexp(userdata)) != 0)
		return ret;
	
	if( (marker_data = hash_data_find(config, HK(marker_data))) != NULL){
		config_freemarkerdata(userdata);
		if( (ret = config_newmarkerdata(userdata, marker_data)) != 0)
			return ret;
	}
	return 0;
} // }}}

static ssize_t regexp_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	char                  *input_str         = NULL;
	regexp_userdata       *userdata          = (regexp_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, input_str, request, userdata->input);
	if(ret != 0 || input_str == NULL)
		return error("no input string in request");
	
	if(regexec(&userdata->regex, input_str, 0, NULL, userdata->eflags) == 0){
		request_t r_next[] = {
			{ userdata->marker, *userdata->marker_data },
			hash_inline(request),
			hash_end
		};
		return (ret = backend_pass(backend, r_next)) < 0 ? ret : -EEXIST;
	}
	return (ret = backend_pass(backend, request)) < 0 ? ret : -EEXIST;
} // }}}

backend_t regexp_proto = {
	.class          = "data/regexp",
	.supported_api  = API_HASH,
	.func_init      = &regexp_init,
	.func_configure = &regexp_configure,
	.func_destroy   = &regexp_destroy,
	.backend_type_hash = {
		.func_handler = &regexp_handler
	}
};

