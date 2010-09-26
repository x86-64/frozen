#line 1 "backends/oid-locator.c.m4"
#include <libfrozen.h>
#include <backend.h>

/* m4 {{{
#line 7


#line 1 "backends/oids/integer.c"
#include <libfrozen.h>

// simply use existing functions


#line 5
        

#line 6
        

#line 7
         

#line 8
        

#line 9 "backends/oid-locator.c.m4"

#line 10 "backends/oid-locator.c.m4"

}}} */
/* oid protos {{{ */
typedef struct oid_proto_t {
	data_type     type;
	f_data_inc    func_increment;
	f_data_div    func_divide;
	f_data_mul    func_multiply;
} oid_proto_t;

oid_proto_t oid_protos[] = {
	{ TYPE_INT8,  &data_int8_inc,  &data_int8_div , &data_int8_mul  },
#line 21
{ TYPE_INT16, &data_int16_inc, &data_int16_div, &data_int16_mul },
#line 21
{ TYPE_INT32, &data_int32_inc, &data_int32_div, &data_int32_mul },
#line 21
{ TYPE_INT64, &data_int64_inc, &data_int64_div, &data_int64_mul },
#line 21

}; // }}}

typedef enum locator_mode {
	LINEAR_INCAPSULATED,  // public oid converted to private oid using div()
	LINEAR_ORIGINAL,      // public oid same as private oid, insecure
	INDEX_INCAPSULATED,   // public oid converted to private oid using index
	INDEX_ORIGINAL        // public oid same as private oid, but checked for existance via index
} locator_mode;

typedef struct locator_ud {
	locator_mode   mode;
	oid_proto_t   *oid_proto;
	unsigned int   linear_scale;
} locator_ud;

static oid_proto_t *  oid_proto_from_type(data_type type){
	int i;
	
	for(i=0; i < sizeof(oid_protos) / sizeof(oid_proto_t); i++){
		if(oid_protos[i].type == type)
			return &(oid_protos[i]);
	}
	return NULL;
}

static int locator_init(chain_t *chain){
	locator_ud *user_data = (locator_ud *)malloc(sizeof(locator_ud));
	if(user_data == NULL)
		return -ENOMEM;
	
	memset(user_data, 0, sizeof(locator_ud));
	
	chain->user_data = user_data;
	return 0;
}

static int locator_destroy(chain_t *chain){
	//locator_ud *data = (locator_ud *)chain->user_data;
	
	free(chain->user_data);
	chain->user_data = NULL;
	return 0;
}

static int locator_configure(chain_t *chain, setting_t *config){
	data_type      oid_class_type;
      //size_type     *oid_class_size_type;
	int            oid_class_supported = 1;
	char          *oid_class_str;
	oid_proto_t   *oid_class_proto;
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
	
	oid_class_type = data_type_from_string(oid_class_str);
	if(oid_class_type == -1)
		return_error(-EINVAL, "locator 'oid-class' invalid\n");
	
	//oid_class_size_type = data_size_type(oid_class_type);
	
	oid_class_proto = oid_proto_from_type(oid_class_type);
	if(oid_class_proto == NULL)
		return_error(-EINVAL, "locator 'oid-class' not supported\n");
	
	linear_scale_str = setting_get_child_string(config, "size");
	linear_scale     = (linear_scale_str != NULL) ? strtoul(linear_scale_str, NULL, 10) : 0;
	
	/* check everything */
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(
				oid_class_proto->func_increment == NULL       ||
				oid_class_proto->func_divide    == NULL       ||
			      //oid_class_size_type             != SIZE_FIXED ||
				linear_scale                    == 0
			)
				oid_class_supported = 0;
			break;
		case INDEX_INCAPSULATED:
			if(oid_class_proto->func_increment == NULL)
				oid_class_supported = 0;
			break;
		case INDEX_ORIGINAL:
		case LINEAR_ORIGINAL:
			// everything ok
			break;
	};
	
	if(oid_class_supported == 0)
		return_error(-EINVAL, "locator 'oid-class' not supported\n");
	
	data->oid_proto    = oid_class_proto;
	data->linear_scale = linear_scale;
	
	// get backend name from 'index' 
	
	return 0;
}

static ssize_t decapsulate(locator_ud *data, request_t *request, char *key){
	data_t      *val;
	data_type    val_type;
	oid_proto_t *val_proto;
	
	if(hash_get_any(request, key, &val_type, &val, NULL) != 0)
		return -EINVAL;
	
	val_proto = oid_proto_from_type(val_type);
	if(val_proto->func_multiply(val, data->linear_scale) != 0)
		return -EINVAL;
	
	return 0;
}

static ssize_t incapsulate(locator_ud *data, buffer_t *buffer){
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
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(decapsulate(data, request, "size") != 0)
				return -EINVAL;
			
			if( (ret = chain_next_query(chain, request, buffer)) <= 0)
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
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(decapsulate(data, request, "key") != 0)
				return -EINVAL;
			
			if( (ret = chain_next_query(chain, request, buffer)) <= 0)
				return ret;
			
			ret = incapsulate_ret(data, ret);
			
			break;
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
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
			ret = chain_next_query(chain, request, buffer);
			break;
	};
	
	return ret;
}

static ssize_t locator_move(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locator_ud  *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(decapsulate(data, request, "key_from") != 0)
				return -EINVAL;
			if(decapsulate(data, request, "key_to")   != 0)
				return -EINVAL;
			if(decapsulate(data, request, "size")     != 0)
				return -EINVAL;
			
			// fall through
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
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
