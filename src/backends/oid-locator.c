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
} oid_proto_t;

oid_proto_t oid_protos[] = {
	{ TYPE_INT8,  &data_int8_inc,  &data_int8_div  },
#line 20
{ TYPE_INT16, &data_int16_inc, &data_int16_div },
#line 20
{ TYPE_INT32, &data_int32_inc, &data_int32_div },
#line 20
{ TYPE_INT64, &data_int64_inc, &data_int64_div },
#line 20

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

static ssize_t locator_create(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t     ret  = -1;
	locator_ud *data = (locator_ud *)chain->user_data;
	data_t     *key;
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			ret = chain_next_query(chain, request, buffer);
			if(ret > 0){
				key = (data_t *)buffer_seek(buffer, 0);
				if(data->oid_proto->func_divide(key, data->linear_scale) != 0){
					// TODO linear incapsulation can't really deal with keys that
					// not cleanly divide on linear_scale. For now we return -EIO,
					// but later need call some pad() to normalize or fix scaling
					
					return -EIO;
				}
			}
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

/*

static ssize_t locator_set(chain_t *chain, request_t *request, buffer_t *buffer){
	int         ret  = -1;
	off_t       ul_key;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case FIXED_STRUCT:
			ul_key = (*(off_t *)key) * data->struct_size; // TODO check overflows
			
			ret = chain_next_crwd_set(chain, &ul_key, value, data->struct_size);
			break;
		case VARIABLE_STRUCT:
			ret = chain_next_crwd_set(chain, key, value, value_size);
			break;
	};
	return ret;
}

static ssize_t locator_get(chain_t *chain, request_t *request, buffer_t *buffer){
	int         ret = -1;
	off_t       ul_key;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case FIXED_STRUCT:
			ul_key = (*(off_t *)key) * data->struct_size; // TODO check overflows
			
			ret = chain_next_crwd_get(chain, &ul_key, value, data->struct_size);
			break;
		case VARIABLE_STRUCT:
			ret = chain_next_crwd_get(chain, key, value, value_size);
			break;
	};
	return ret;
}

static ssize_t locator_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	int         ret  = -1;
	off_t       ul_key;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case FIXED_STRUCT:
			ul_key = (*(off_t *)key) * data->struct_size; // TODO check overflows
			
			ret = chain_next_crwd_delete(chain, &ul_key, data->struct_size);
			break;
		case VARIABLE_STRUCT:
			ret = chain_next_crwd_delete(chain, key, value_size);
			break;
	};
	return ret;
}

static ssize_t locator_move(chain_t *chain, request_t *request, buffer_t *buffer){
	int         ret = -1;
	off_t       ul_keyf, ul_keyt;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case FIXED_STRUCT:
			ul_keyf = (*(off_t *)key_from) * data->struct_size; // TODO check overflows
			ul_keyt = (*(off_t *)key_to)   * data->struct_size;
			
			ret = chain_next_crwd_move(chain, &ul_keyf, &ul_keyt, value_size * data->struct_size);
			break;
		case VARIABLE_STRUCT:
			return chain_next_crwd_move(chain, key_from, key_to, value_size);
			break;
	};
	return ret;
}

static ssize_t locator_count(chain_t *chain, request_t *request, buffer_t *buffer){
	int         ret  = -1;
	locator_ud *data = (locator_ud *)chain->user_data;
	
	switch(data->mode){
		case FIXED_STRUCT:
			ret = chain_next_crwd_count(chain, count);
			if(ret == 0){
				*(size_t *)count = (*(size_t *)count) / data->struct_size;
			}
			break;
		case VARIABLE_STRUCT:
			return chain_next_crwd_count(chain, count);
			break;
	};
	return ret;
}
*/

static chain_t chain_locator = {
	"oid-locator",
	CHAIN_TYPE_CRWD,
	&locator_init,
	&locator_configure,
	&locator_destroy,
	{{
		.func_create = &locator_create,
	}}
};
CHAIN_REGISTER(chain_locator)

/* vim: set filetype=c: */
