#include <libfrozen.h>
#include <backend.h>

typedef enum locator_mode {
	LINEAR_INCAPSULATED,  // public oid converted to private oid using div()
	LINEAR_ORIGINAL,      // public oid same as private oid, insecure
	INDEX_INCAPSULATED,   // public oid converted to private oid using index
	INDEX_ORIGINAL        // public oid same as private oid, but checked for existance via index
} locator_mode;

typedef struct locator_ud {
	locator_mode   mode;
	data_type      oid_type;
	unsigned int   linear_scale;
	hash_t        *new_request;
	
} locator_ud;

static int locator_init(chain_t *chain){
	locator_ud *user_data = (locator_ud *)calloc(1, sizeof(locator_ud));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	return 0;
}

static int locator_destroy(chain_t *chain){
	locator_ud *data = (locator_ud *)chain->user_data;
	
	hash_free(data->new_request);
	
	free(data);
	chain->user_data = NULL;
	return 0;
}

static int locator_configure(chain_t *chain, setting_t *config){
      //size_type     *oid_class_size_type;
	int            oid_class_supported = 1;
	char          *oid_class_str;
	char          *mode_str;
	char          *linear_scale_str;
	unsigned int   linear_scale;
	locator_ud    *data                = (locator_ud *)chain->user_data;
	
	mode_str = setting_get_child_string(config, "mode");
	if(mode_str == NULL)
		return_error(-EINVAL, "locator 'mode' not defined\n");
	
	      if(strcasecmp(mode_str, "linear-incapsulated") == 0){
		data->mode = LINEAR_INCAPSULATED;
	}else if(strcasecmp(mode_str, "linear-original") == 0){
		data->mode = LINEAR_ORIGINAL;
	}else if(strcasecmp(mode_str, "index-incapsulated") == 0){
		data->mode = INDEX_INCAPSULATED;
	}else if(strcasecmp(mode_str, "index-original") == 0){
		data->mode = INDEX_ORIGINAL;
	}else{
		return_error(-EINVAL, "locator 'mode' invalid or not supported\n");
	}
	
	oid_class_str = setting_get_child_string(config, "oid-class");
	if(oid_class_str == NULL)
		return_error(-EINVAL, "locator 'oid-class' not defined\n");
	
	if( (data->oid_type = data_type_from_string(oid_class_str)) == -1)
		return_error(-EINVAL, "locator 'oid-class' invalid\n");
	
	linear_scale_str = setting_get_child_string(config, "size");
	linear_scale     = (linear_scale_str != NULL) ? strtoul(linear_scale_str, NULL, 10) : 0;
	
	/* check everything */
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(
			      //oid_class_proto->func_add       == NULL       ||
			      //oid_class_proto->func_divide    == NULL       ||
			      //oid_class_size_type             != SIZE_FIXED ||
				linear_scale                    == 0
			)
				oid_class_supported = 0;
			break;
		case INDEX_INCAPSULATED:
			//if(oid_class_proto->func_add == NULL)
			//	oid_class_supported = 0;
			break;
		case INDEX_ORIGINAL:
		case LINEAR_ORIGINAL:
			// everything ok
			break;
	};
	
	if(oid_class_supported == 0)
		return_error(-EINVAL, "locator 'oid-class' not supported\n");
	
	data->linear_scale = linear_scale;
	data->new_request  = hash_new();
	
	// get backend name from 'index' 
	
	return 0;
}

static ssize_t decapsulate(locator_ud *data, request_t *request, char *key){
	data_t       *val;
	data_type     val_type;
	data_proto_t *val_proto;
	
	// TODO holly crap
	
	if(hash_get_any(request, key, &val_type, &val, NULL) != 0)
		return -EINVAL;
	
	if(hash_set(data->new_request, key, val_type, val) != 0)
		return -EINVAL;
	
	if(hash_get_any(data->new_request, key, NULL, &val, NULL) != 0)
		return -EINVAL;
	
	val_proto = data_proto_from_type(val_type);
	if(val_proto->func_mul(val, data->linear_scale) != 0)
		return -EINVAL;
	
	return 0;
}

static ssize_t incapsulate(locator_ud *data, buffer_t *buffer){
	/*
	data_t      *key;
	
	// divide returned key on data->size
	key = (data_t *)buffer_seek(buffer, 0);
	if(data->oid_proto->func_divide(key, data->linear_scale) != 0){
				
		// TODO linear incapsulation can't really deal with keys that
		// not cleanly divide on linear_scale. For now we return -EIO,
		// but later need call some pad() to normalize or fix scaling
		
		return -EIO;
	}
	return 0;
	*/
	return -1;
}

static ssize_t incapsulate_ret(locator_ud *data, ssize_t ret){
	div_t  div_res;
	div_res = div(ret, data->linear_scale);
	
	if(div_res.rem != 0)             // return rounded up integer, coz user can write smaller data than size of one item
		return div_res.quot + 1;
	
	return div_res.quot;
}

static ssize_t locator_create(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locator_ud  *data = (locator_ud *)chain->user_data;
	
	hash_empty        (data->new_request);
	hash_assign_layer (data->new_request, request);
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(decapsulate(data, request, "size") != 0)
				return -EINVAL;
			
			if( (ret = chain_next_query(chain, data->new_request, buffer)) <= 0)
				return ret;
			
			if(incapsulate(data, buffer) != 0)
				return -EIO;
			
			break;
		
		case INDEX_INCAPSULATED:
			// ret = check_existance(key, buffer);
			// if(ret <= 0)
			// 	return ret;
			//
			// hash_set(request, "key", *buffer, ret);
			// ret = chain_next_query(chain, request, buffer);
			break;
		
		case INDEX_ORIGINAL:
			// ret = check_existance(key);
			// if(ret != 0)
			// 	return ret;
			
			// fall through
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
			break;
	};
	
	return ret;
}

static ssize_t locator_setgetdelete(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locator_ud  *data = (locator_ud *)chain->user_data;
	
	hash_empty        (data->new_request);
	hash_assign_layer (data->new_request, request);
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(decapsulate(data, request, "key") != 0)
				return -EINVAL;
			if(decapsulate(data, request, "size") != 0)
				return -EINVAL;
			
			if( (ret = chain_next_query(chain, data->new_request, buffer)) <= 0)
				return ret;
			
			ret = incapsulate_ret(data, ret);
			break;
			
		case INDEX_INCAPSULATED:
			// ret = check_existance(key, buffer);
			// if(ret <= 0)
			// 	return ret;
			//
			// hash_set(request, "key", *buffer, ret);
			
			ret = chain_next_query(chain, request, buffer);
			break;
			
		case INDEX_ORIGINAL:
			// ret = check_existance(key);
			// if(ret != 0)
			// 	return ret;
			
			// fall through
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
			break;
	};
	
	return ret;
}

static ssize_t locator_move(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locator_ud  *data = (locator_ud *)chain->user_data;
	
	hash_empty        (data->new_request);
	hash_assign_layer (data->new_request, request);
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(decapsulate(data, request, "key_from") != 0)
				return -EINVAL;
			if(decapsulate(data, request, "key_to")   != 0)
				return -EINVAL;
			decapsulate(data, request, "size");             // optional
			
			ret = chain_next_query(chain, data->new_request, buffer);
			break;
			
		case INDEX_INCAPSULATED:
			// ret = check_existance(key, buffer);
			// if(ret <= 0)
			// 	return ret;
			//
			// hash_set(request, "key", *buffer, ret);
			
			ret = chain_next_query(chain, data->new_request, buffer);
			break;
		
		case INDEX_ORIGINAL:
			// ret = check_existance(key);
			// if(ret != 0)
			// 	return ret;
			
			// fall through
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
			break;
	};
	
	return ret;
}

static ssize_t locator_count(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locator_ud  *data = (locator_ud *)chain->user_data;
	
	if( (ret = chain_next_query(chain, request, buffer)) <= 0)
		return ret;
	
	if(incapsulate(data, buffer) != 0)
		return -EIO;
	
	return ret;
}

static chain_t chain_locator = {
	"oid-locator",
	CHAIN_TYPE_CRWD,
	&locator_init,
	&locator_configure,
	&locator_destroy,
	{{
		.func_create = &locator_create,
		.func_set    = &locator_setgetdelete,
		.func_get    = &locator_setgetdelete,
		.func_delete = &locator_setgetdelete,
		.func_move   = &locator_move,
		.func_count  = &locator_count,
	}}
};
CHAIN_REGISTER(chain_locator)

/* vim: set filetype=c: */
