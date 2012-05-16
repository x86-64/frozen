#include <libfrozen.h>

ssize_t              helper_key_current     (data_t *key, data_t *token){ // {{{
	data_t          d_key    = DATA_UINTT(0);
	fastcall_lookup r_lookup = { { 4, ACTION_LOOKUP }, &d_key, token };
	return data_query(key, &r_lookup);
} // }}}

