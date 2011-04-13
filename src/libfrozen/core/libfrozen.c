/**
 * @file libfrozen.c
 * @brief Library main functions
 */

#include "libfrozen.h"

hash_t *global_settings; ///< Global settings

/** @brief Initialize library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_init(void){
	hash_t  global_proto[20] = {
		[0 ... 18] = hash_null,
		hash_end
	};
	
	global_settings = malloc(sizeof(global_proto));
	memcpy(global_settings, &global_proto, sizeof(global_proto));
	
	srandom(time(NULL));
	return 0;
}

/** @brief Destroy library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_destroy(void){
	backend_destroy_all();
	free(global_settings);
	return 0;
}

intmax_t  safe_pow(uintmax_t *res, uintmax_t x, uintmax_t y){
	uintmax_t t;
	
	if(y == 0){ t = 1; goto exit; }
	if(y == 1){ t = x; goto exit; }
	
	t = x;
	while(y-- >= 0){
		if(__MAX(uintmax_t) / x <= t)
			return -EINVAL;
		
		t *= x;
	}
exit:
	*res = t;
	return 0;
}

intmax_t safe_mul(uintmax_t *res, uintmax_t x, uintmax_t y){
	if(x == 0 || y == 0){
		*res = 0;
		return 0;
	}
	
	if(__MAX(uintmax_t) / x <= y)
		return -EINVAL;
	
	*res = x * y;
	return 0;
}

intmax_t safe_div(uintmax_t *res, uintmax_t x, uintmax_t y){
	if(y == 0)
		return -EINVAL;
	
	*res = x / y;
	return 0;
}

