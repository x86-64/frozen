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

