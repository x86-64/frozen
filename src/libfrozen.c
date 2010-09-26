#include "libfrozen.h"

setting_t *global_settings;

int frozen_init(void){
	global_settings = setting_new();
}

int frozen_destory(void){
	setting_destroy(global_settings);
}

