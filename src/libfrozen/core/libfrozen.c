/**
 * @file libfrozen.c
 * @brief Library main functions
 */

#include "libfrozen.h"

/** @brief Initialize library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_init(void){
	ssize_t                ret;
	srandom(time(NULL));
	
	if( (ret = frozen_data_init()) != 0)
		return ret;
	
	if( (ret = frozen_backend_init()) != 0)
		return ret;
	
	return 0;
}

/** @brief Destroy library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_destroy(void){
	frozen_backend_destroy();
	frozen_data_destroy();
	return 0;
}

intmax_t  safe_pow(uintmax_t *res, uintmax_t x, uintmax_t y){
	uintmax_t t;
	
	if(y == 0){ t = 1; goto exit; }
	if(y == 1){ t = x; goto exit; }
	
	t = x;
	while(--y >= 1){
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

