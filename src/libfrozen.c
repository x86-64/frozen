#include "libfrozen.h"

setting_t *global_settings;

int frozen_init(void){
	global_settings = setting_new();
	return 0;
}

int frozen_destroy(void){
	setting_destroy(global_settings);
	return 0;
}

