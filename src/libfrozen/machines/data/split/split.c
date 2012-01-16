#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_split data/split
 */
/**
 * @ingroup mod_machine_split
 * @page page_split_info Description
 *
 * This module split incoming buffer using supplied separator and call underlying machine with every chunk.
 */
/**
 * @ingroup mod_machine_split
 * @page page_split_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/split",
 *              input                   = (hashkey_t)'buffer',  # input buffer key name
 *              split                   = "Z",                  # string to split on, default "\n"
 *              buffer_size             = (uint_t)'1024',       # size of internal buffer, default 1024
 *              dump_last               = (uint_t)'1'           # dump last part of incoming buffer
 *                                                              #   (overwise it would be collected for further request), default 0
 * }
 * @endcode
 */

#define EMODULE 35

typedef struct split_userdata {
	char                  *split_str;
	uintmax_t              split_len;
	hashkey_t              input;
	uintmax_t              buffer_size;
	uintmax_t              dump_last;

	thread_data_ctx_t      thread_data;
} split_userdata;

typedef struct split_threaddata {
	char                  *buffer;

	uintmax_t              last_part_filled;
	container_t            last_part;
} split_threaddata;

static void * split_threaddata_create(split_userdata *userdata){ // {{{
	split_threaddata      *threaddata;
	
	if( (threaddata = malloc(sizeof(split_threaddata))) == NULL)
		goto error1;
	
	if( (threaddata->buffer = malloc(userdata->buffer_size)) == NULL)
		goto error2;
	
	container_init(&threaddata->last_part);
	
	threaddata->last_part_filled = 0;
	return threaddata;

error2:
	free(threaddata);
error1:
	return NULL;
} // }}}
static void   split_threaddata_destroy(split_threaddata *threaddata){ // {{{
	container_destroy(&threaddata->last_part);
	free(threaddata->buffer);
	free(threaddata);
} // }}}

static int split_init(machine_t *machine){ // {{{
	split_userdata       *userdata;

	if((userdata = machine->userdata = calloc(1, sizeof(split_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->input       = HK(buffer);
	userdata->split_str   = strdup("\n");
	userdata->split_len   = 1;
	userdata->buffer_size = 1024;
	userdata->dump_last   = 0;
	
	return thread_data_init(
		&userdata->thread_data, 
		(f_thread_create)&split_threaddata_create,
		(f_thread_destroy)&split_threaddata_destroy,
		userdata);
} // }}}
static int split_destroy(machine_t *machine){ // {{{
	split_userdata        *userdata          = (split_userdata *)machine->userdata;
	
	thread_data_destroy(&userdata->thread_data);
	
	free(userdata->split_str);
	free(userdata);
	return 0;
} // }}}
static int split_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	char                  *split_str;
	split_userdata        *userdata          = (split_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,        config, HK(input));
	hash_data_copy(ret, TYPE_UINTT,    userdata->buffer_size,  config, HK(buffer_size));
	hash_data_copy(ret, TYPE_UINTT,    userdata->dump_last,    config, HK(dump_last));
	
	hash_data_copy(ret, TYPE_STRINGT,  split_str,              config, HK(split));
	if(ret == 0){
		free(userdata->split_str);
		userdata->split_str = strdup(split_str);
		userdata->split_len = strlen(split_str);
	}
	
	userdata->buffer_size = MAX(userdata->buffer_size, userdata->split_len);
	return 0;
} // }}}

static ssize_t split_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *input;
	char                  *buffer;
	char                  *match;
	uintmax_t              buffer_left;
	uintmax_t              lmatch_offset     = 0;
	uintmax_t              original_offset   = 0;
	uintmax_t              original_eoffset;
	split_userdata        *userdata          = (split_userdata *)machine->userdata;
	split_threaddata      *threaddata        = thread_data_get(&userdata->thread_data);
	
	if( (input = hash_data_find(request, userdata->input)) == NULL)
		return error("no input string in request");
	
	data_t                 hslider           = DATA_SLIDERT(input, 0);
	
	while(1){
		fastcall_read r_read = { { 5, ACTION_READ }, 0, threaddata->buffer, userdata->buffer_size };
		if(data_query(&hslider, &r_read) != 0)
			break;
		
		buffer            = threaddata->buffer;
		buffer_left       = r_read.buffer_size;
		original_eoffset  = original_offset + r_read.buffer_size;

		while(1){
			if( (match = memchr(buffer, (int)userdata->split_str[0], buffer_left)) == NULL)
				break;
			
			buffer_left -= (match - buffer);
			
			if(buffer_left < userdata->split_len){
				memcpy(threaddata->buffer, match, buffer_left);
				
				fastcall_read r_read = { { 5, ACTION_READ }, 0, threaddata->buffer + buffer_left, userdata->buffer_size - buffer_left };
				if(data_query(&hslider, &r_read) != 0)
					break;
				
				buffer            = threaddata->buffer;
				buffer_left      += r_read.buffer_size;
				original_offset  += (match - threaddata->buffer);
				original_eoffset  = original_offset + r_read.buffer_size;
				continue;
			}
			
			if(memcmp(match, userdata->split_str, userdata->split_len) == 0){
				uintmax_t       match_offset     = original_offset + (match - threaddata->buffer) + userdata->split_len;
				data_t          hslice           = DATA_SLICET(input, lmatch_offset, match_offset - lmatch_offset);
				
				if(threaddata->last_part_filled == 1){
					container_add_tail_data(&threaddata->last_part, &hslice, CHUNK_ADOPT_DATA | CHUNK_DONT_FREE);
					
					request_t r_next[] = {
						{ userdata->input, DATA_PTR_CONTAINERT(&threaddata->last_part) },
						hash_inline(request),
						hash_end
					};
					if( (ret = machine_pass(machine, r_next)) < 0)
						return ret;
					
					container_clean(&threaddata->last_part);
					threaddata->last_part_filled = 0;
				}else{
					request_t r_next[] = {
						{ userdata->input, hslice },
						hash_inline(request),
						hash_end
					};
					if( (ret = machine_pass(machine, r_next)) < 0)
						return ret;
				}
				
				lmatch_offset = match_offset;
			}

			buffer         = match + 1;
			buffer_left   -= 1;
		}
		
		original_offset = original_eoffset;
	}
	data_t    hslice           = DATA_SLICET(input, lmatch_offset, ~0);
	
	if(userdata->dump_last != 0){
		request_t r_next[] = {
			{ userdata->input, hslice },
			hash_inline(request),
			hash_end
		};
		if( (ret = machine_pass(machine, r_next)) < 0)
			return ret;
	}else{
		// save end of buffer to use in next request
		data_t         last_part         = { TYPE_RAWT, NULL };
		
		fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &last_part };
		if(data_query(&hslice, &r_transfer) >= 0){
			container_add_tail_data(&threaddata->last_part, &last_part, CHUNK_ADOPT_DATA);
			threaddata->last_part_filled = 1;
		}
	}
	
	return 0;
} // }}}

machine_t split_proto = {
	.class          = "data/split",
	.supported_api  = API_HASH,
	.func_init      = &split_init,
	.func_configure = &split_configure,
	.func_destroy   = &split_destroy,
	.machine_type_hash = {
		.func_handler = &split_handler
	}
};

