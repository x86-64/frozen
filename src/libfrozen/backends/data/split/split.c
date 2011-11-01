#include <libfrozen.h>
#include <alloca.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_split Backend 'data/split'
 */
/**
 * @ingroup mod_backend_split
 * @page page_split_info Description
 *
 * This module split incoming buffer using supplied separator and call underlying backend with every chunk.
 */
/**
 * @ingroup mod_backend_split
 * @page page_split_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/split",
 *              split                   = "aaa.*",            # split for matching, default ".*"
 * }
 * @endcode
 */

#define EMODULE 35

typedef struct split_userdata {
	char                  *split_str;
	uintmax_t              split_len;
	hash_key_t             input;
	uintmax_t              buffer_size;
} split_userdata;

static int split_init(backend_t *backend){ // {{{
	split_userdata       *userdata;

	if((userdata = backend->userdata = calloc(1, sizeof(split_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->input       = HK(buffer);
	userdata->split_str   = strdup("\n");
	userdata->split_len   = 1;
	userdata->buffer_size = 1024;
	return 0;
} // }}}
static int split_destroy(backend_t *backend){ // {{{
	split_userdata        *userdata          = (split_userdata *)backend->userdata;
	
	free(userdata->split_str);
	free(userdata);
	return 0;
} // }}}
static int split_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	char                  *split_str;
	split_userdata        *userdata          = (split_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,        config, HK(input));
	hash_data_copy(ret, TYPE_UINTT,    userdata->buffer_size,  config, HK(buffer_size));
	
	hash_data_copy(ret, TYPE_STRINGT,  split_str,              config, HK(split));
	if(ret == 0){
		free(userdata->split_str);
		userdata->split_str = strdup(split_str);
		userdata->split_len = strlen(split_str);
	}
	
	userdata->buffer_size = MAX(userdata->buffer_size, userdata->split_len);
	return 0;
} // }}}

static ssize_t split_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *input;
	char                  *buffer;
	char                  *match;
	char                  *tmp;
	uintmax_t              buffer_left;
	uintmax_t              lmatch_offset     = 0;
	uintmax_t              original_offset   = 0;
	uintmax_t              original_eoffset;
	split_userdata        *userdata          = (split_userdata *)backend->userdata;
	
	tmp = alloca(userdata->buffer_size);
	
	if( (input = hash_data_find(request, userdata->input)) == NULL)
		return error("no input string in request");
	
	data_t                 hslider           = DATA_SLIDERT(input, 0);
	
	while(1){
		fastcall_read r_read = { { 5, ACTION_READ }, 0, tmp, userdata->buffer_size };
		if(data_query(&hslider, &r_read) != 0)
			break;
		
		buffer            = tmp;
		buffer_left       = r_read.buffer_size;
		original_eoffset  = original_offset + r_read.buffer_size;

		while(1){
			if( (match = memchr(buffer, (int)userdata->split_str[0], buffer_left)) == NULL)
				break;
			
			buffer_left -= (match - buffer);
			
			if(buffer_left < userdata->split_len){
				memcpy(tmp, match, buffer_left);
				
				fastcall_read r_read = { { 5, ACTION_READ }, 0, tmp + buffer_left, userdata->buffer_size - buffer_left };
				if(data_query(&hslider, &r_read) != 0)
					break;
				
				buffer            = tmp;
				buffer_left      += r_read.buffer_size;
				original_offset  += (match - tmp);
				original_eoffset  = original_offset + r_read.buffer_size;
				continue;
			}
			
			if(memcmp(match, userdata->split_str, userdata->split_len) == 0){
				uintmax_t       match_offset     = original_offset + (match - tmp) + userdata->split_len;
				data_t          hslice           = DATA_SLICET(input, lmatch_offset, match_offset - lmatch_offset);
				
				request_t r_next[] = {
					{ userdata->input, hslice },
					hash_inline(request),
					hash_end
				};
				if( (ret = backend_pass(backend, r_next)) < 0)
					return ret;
				
				lmatch_offset = match_offset;
			}

			buffer         = match + 1;
			buffer_left   -= 1;
		}
		
		original_offset = original_eoffset;
	}
	
	data_t    hslice           = DATA_SLICET(input, lmatch_offset, ~0);
	request_t r_next[] = {
		{ userdata->input, hslice },
		hash_inline(request),
		hash_end
	};
	if( (ret = backend_pass(backend, r_next)) < 0)
		return ret;
	
	return 0;
} // }}}

backend_t split_proto = {
	.class          = "data/split",
	.supported_api  = API_HASH,
	.func_init      = &split_init,
	.func_configure = &split_configure,
	.func_destroy   = &split_destroy,
	.backend_type_hash = {
		.func_handler = &split_handler
	}
};

