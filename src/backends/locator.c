#include <libfrozen.h>
#include <backend.h>

typedef enum locator_mode {
	LINEAR_STRUCT,
	DYNAMIC_STRUCT
} locator_mode;

typedef struct locator_ud {
	locator_mode   mode;
	size_t         struct_size;
} locator_ud;

static int locator_init(chain_t *chain){
	locator_ud *user_data = malloc(sizeof(locator_ud));
	if(user_data == NULL)
		return -ENOMEM;
	
	memset(user_data, 0, sizeof(locator_ud));
	
	chain->user_data = user_data;
	
	return 0;
}

static int locator_destroy(chain_t *chain){
	locator_ud *data = (locator_ud *)chain->user_data;
	
	free(chain->user_data);
	
	chain->user_data = NULL;
	
	return 0;
}

static int locator_configure(chain_t *chain, setting_t *config){
	char * temp;
	
	locator_ud *data = (locator_ud *)chain->user_data;
	
	temp = setting_get_child_string(config, "struct_size");
	if(temp != NULL){
		data->struct_size = strtoul(temp, NULL, 10);
		data->mode        = LINEAR_STRUCT;
		
		goto mode_parsed;
	}
	
	temp = setting_get_child_string(config, "struct");
	// TODO
	
	return -EINVAL;
mode_parsed:
	
	
	return 0;
}


static int locator_create(chain_t *chain, void *key, size_t value_size){
	int         ret;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case LINEAR_STRUCT:
			ret = chain_next_crwd_create(chain, key, value_size * data->struct_size);
			if(ret == 0){
				*(off_t *)key = (*(off_t *)key) / data->struct_size;
			}
			break;
		case DYNAMIC_STRUCT:
			ret = chain_next_crwd_create(chain, key, value_size);
			break;
	};
	return ret;
}

static int locator_set(chain_t *chain, void *key, buffer_t *value, size_t value_size){
	int         ret;
	off_t       ul_key;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case LINEAR_STRUCT:
			ul_key = (*(off_t *)key) * data->struct_size; // TODO check overflows
			
			ret = chain_next_crwd_set(chain, &ul_key, value, data->struct_size);
			break;
		case DYNAMIC_STRUCT:
			ret = chain_next_crwd_set(chain, key, value, value_size);
			break;
	};
	return ret;
}

static int locator_get(chain_t *chain, void *key, buffer_t *value, size_t value_size){
	int         ret;
	off_t       ul_key;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case LINEAR_STRUCT:
			ul_key = (*(off_t *)key) * data->struct_size; // TODO check overflows
			
			ret = chain_next_crwd_get(chain, &ul_key, value, data->struct_size);
			break;
		case DYNAMIC_STRUCT:
			ret = chain_next_crwd_get(chain, key, value, value_size);
			break;
	};
	return ret;
}

static int locator_delete(chain_t *chain, void *key, size_t value_size){
	int         ret;
	off_t       ul_key;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case LINEAR_STRUCT:
			ul_key = (*(off_t *)key) * data->struct_size; // TODO check overflows
			
			ret = chain_next_crwd_delete(chain, &ul_key, data->struct_size);
			break;
		case DYNAMIC_STRUCT:
			ret = chain_next_crwd_delete(chain, key, value_size);
			break;
	};
	return ret;
}

static int locator_move(chain_t *chain, void *key_from, void *key_to, size_t value_size){
	int         ret;
	off_t       ul_keyf, ul_keyt;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case LINEAR_STRUCT:
			ul_keyf = (*(off_t *)key_from) * data->struct_size; // TODO check overflows
			ul_keyt = (*(off_t *)key_to)   * data->struct_size;
			
			ret = chain_next_crwd_move(chain, &ul_keyf, &ul_keyt, value_size * data->struct_size);
			break;
		case DYNAMIC_STRUCT:
			return chain_next_crwd_move(chain, key_from, key_to, value_size);
			break;
	};
	return ret;
}

static int locator_count(chain_t *chain, void *count){
	int         ret;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case LINEAR_STRUCT:
			ret = chain_next_crwd_count(chain, count);
			if(ret == 0){
				*(size_t *)count = (*(size_t *)count) / data->struct_size;
			}
			break;
		case DYNAMIC_STRUCT:
			return chain_next_crwd_count(chain, count);
			break;
	};
	return ret;
}

static chain_t chain_locator = {
	"locator",
	CHAIN_TYPE_CRWD,
	&locator_init,
	&locator_configure,
	&locator_destroy,
	{
		&locator_create,
		&locator_set,
		&locator_get,
		&locator_delete,
		&locator_move,
		&locator_count
	}
};
CHAIN_REGISTER(chain_locator)

