/**
 * @file libfrozen.c
 * @brief Library main functions
 */

#include "libfrozen.h"

uintmax_t inited = 0;

/** @brief Initialize library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_init(void){
	ssize_t                ret;
	
	if(inited == 1)
		return 0;

	srandom(time(NULL));
	
	if( (ret = frozen_data_init()) != 0)
		return ret;
	
	if( (ret = frozen_machine_init()) != 0)
		return ret;
	
	inited = 1;
	return 0;
}

/** @brief Destroy library
 * @return 0 on success
 * @return -1 on error
 */
int frozen_destroy(void){
	if(inited == 0)
		return 0;
	
	frozen_machine_destroy();
	frozen_data_destroy();
	
	inited = 0;
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

void * memdup(void *ptr, uintmax_t size){
	void                  *new_ptr;

	if( (new_ptr = malloc(size)) == NULL)
		return NULL;
	memcpy(new_ptr, ptr, size);
	return new_ptr;
}

uintmax_t portable_hash(char *p){
	register char          c;
	register uintmax_t     i                 = 1;
	register uintmax_t     key_val           = 0;
	
	while((c = *p++)){
		key_val += c * i * i;
		i++;
	}
	return key_val;
}
