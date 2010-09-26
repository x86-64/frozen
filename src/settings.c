#include <libfrozen.h>
#include <settings.h>

/* private */
static void        setting_set_list(setting_t *setting){
	setting_empty(setting);
	
	setting->type = SETTING_LIST;

	list_init(&setting->list.list);
}
static int         setting_iter_find_child(void *p_setting, void *p_name, void *p_child){
	if(strcasecmp(((setting_t *)p_setting)->name, p_name) == 0){
		*(setting_t **)p_child = (setting_t *)p_setting;
		
		return ITER_BREAK;
	}
	return ITER_CONTINUE;
}
static setting_t * setting_find_child(setting_t *setting, char *name){
	setting_t *child = NULL;
	
	if(setting == NULL)
		return NULL;
	
	if(setting->type != SETTING_LIST){
		setting_set_list(setting);
		
		return NULL;
	}
	
	if(name == NULL)
		return NULL;
	
	list_iter(&setting->list.list, (iter_callback)&setting_iter_find_child, name, &child);
	
	return child;
}

/* public */
setting_t *  setting_new(void){
	setting_t *setting = (setting_t *)malloc(sizeof(setting_t));
	
	memset(setting, 0, sizeof(setting_t));
	
	setting->type = SETTING_NONE;
	
	return setting;
}
setting_t *  setting_new_named(char *name){
	setting_t *setting = setting_new();
	
	setting->name = strdup(name);
	
	return setting;
}
void         setting_destroy(setting_t *setting){
	if(setting == NULL)
		return;
	
	setting_empty(setting);
	
	if(setting->name != NULL)
		free(setting->name);

	free(setting);
}
void         setting_empty(setting_t *setting){
	setting_t *child;
	
	switch(setting->type){
		case SETTING_STRING:
			setting->type = SETTING_NONE;
			
			if(setting->string.value != NULL)
				free(setting->string.value);
			
			break;
		case SETTING_LIST:
			while(child = list_pop(&setting->list.list))
				setting_destroy(child);
			
			list_destroy(&setting->list.list);
			
			break;
		case SETTING_NONE:
		default:
			// do nothing
			break;
	};
}

void         setting_set_string(setting_t *setting, char *value){
	setting_empty(setting);
	
	setting->type         = SETTING_STRING;
	if(value != NULL){
		setting->string.value = strdup(value);
	}else{
		setting->string.value = NULL;
	}
}
char *       setting_get_string(setting_t *setting){
	if(setting->type != SETTING_STRING)
		return NULL;
	
	return setting->string.value;
}

int          setting_iter_child(setting_t *setting, iter_callback func, void *arg1, void *arg2){
	if(setting->type != SETTING_LIST)
		return -EINVAL;
	
	return list_iter(&setting->list.list, func, arg1, arg2);
}
void         setting_set_child_string(setting_t *setting, char *name, char *value){
	
	setting_t *child = setting_find_child(setting, name);
	if(child == NULL){
		child = setting_new_named(name);
		if(child == NULL)
			return;
		
		list_push(&setting->list.list, child);
	}
	
	setting_set_string(child, value);
}
char *       setting_get_child_string(setting_t *setting, char *name){
	
	setting_t *child = setting_find_child(setting, name);
	if(child == NULL)
		return NULL;
	
	return setting_get_string(child);
}
void         setting_set_child_setting(setting_t *setting, setting_t *child){
	
	setting_t *old_child = setting_find_child(setting, child->name);
		
	if(old_child != NULL)
		setting_destroy_child(setting, old_child->name);
	
	list_push(&setting->list.list, child);
}
setting_t *  setting_get_child_setting(setting_t *setting, char *name){
	return setting_find_child(setting, name);
}
void         setting_destroy_child(setting_t *setting, char *name){
	
	setting_t *child = setting_find_child(setting, name);
	if(child != NULL){
		list_unlink(&setting->list.list, child);
		
		setting_destroy(child);
	}
}

