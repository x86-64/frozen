#include <libfrozen.h>
#include <backend.h>
#include <alloca.h>

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
	data_ctx_t     ctx;

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
	
	data_ctx_destroy(&data->ctx);
	
	free(data);
	chain->user_data = NULL;
	return 0;
}

static int locator_configure(chain_t *chain, hash_t *config){
      //size_type     *oid_class_size_type;
	int            oid_class_supported = 1;
	char          *oid_class_str;
	char          *mode_str;
	char          *linear_scale_str;
	unsigned int   linear_scale;
	locator_ud    *data                = (locator_ud *)chain->user_data;
	
	if(hash_get_typed(config, TYPE_STRING, "mode", (void **)&mode_str, NULL) != 0)
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
	
	if(hash_get_typed(config, TYPE_STRING, "oid-class", (void **)&oid_class_str, NULL) != 0)
		return_error(-EINVAL, "locator 'oid-class' not defined\n");
	
	if( (data->oid_type = data_type_from_string(oid_class_str)) == -1)
		return_error(-EINVAL, "locator 'oid-class' invalid\n");
	
	linear_scale =
		(hash_get_typed(config, TYPE_INT32, "size", (void **)&linear_scale_str, NULL) == 0) ?
		*(unsigned int *)linear_scale_str : 0;
	
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
	
	data_ctx_init(&data->ctx, data->oid_type, NULL); 
	
	data->linear_scale = linear_scale;
	
	// get backend name from 'index' 
	
	return 0;
}

static ssize_t incapsulate(locator_ud *data, buffer_t *buffer){
	// divide returned key on data->size
	if(data_buffer_arithmetic(&data->ctx, buffer, 0, '/', data->linear_scale) != 0){
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
	hash_t      *r_size;
	void        *o_size;
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if( (r_size = hash_find(request, "size")) == NULL)
				return -EINVAL;
			
			o_size = alloca(r_size->value_size);
			memcpy(o_size, r_size->value, r_size->value_size);
			if(data_bare_arithmetic(r_size->type, o_size, '*', data->linear_scale) != 0)
				return -EINVAL;
			
			hash_t  new_request[] = {
				{ "size", r_size->type, o_size, r_size->value_size },
				hash_next(request)
			};
			
			if( (ret = chain_next_query(chain, new_request, buffer)) <= 0)
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
	hash_t      *r_key, *r_size;
	void        *o_key, *o_size;
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if( (r_key  = hash_find(request, "key"))  == NULL) return -EINVAL;
			if( (r_size = hash_find(request, "size")) == NULL) return -EINVAL;
			
			o_key  = alloca(r_key->value_size);
			o_size = alloca(r_size->value_size);
			memcpy(o_key,  r_key->value,  r_key->value_size);
			memcpy(o_size, r_size->value, r_size->value_size);
			
			if(data_bare_arithmetic(r_key->type,  o_key,  '*', data->linear_scale) != 0) return -EINVAL;
			if(data_bare_arithmetic(r_size->type, o_size, '*', data->linear_scale) != 0) return -EINVAL;
			
			hash_t  new_request[] = {
				{ "key",  r_key->type,  o_key,  r_key->value_size  },
				{ "size", r_size->type, o_size, r_size->value_size },
				hash_next(request)
			};
			
			if( (ret = chain_next_query(chain, new_request, buffer)) <= 0)
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
	hash_t      *r_key_from, *r_key_to;
	hash_t      *r_size;
	char        *r_size_str = hash_ptr_null;
	void        *o_key_from, *o_key_to, *o_size;
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if( (r_key_from = hash_find(request, "key_from")) == NULL) return -EINVAL;
			if( (r_key_to   = hash_find(request, "key_to"))   == NULL) return -EINVAL;
			
			if( (r_size     = hash_find(request, "size")) != NULL){ // size is optional
				o_size     = alloca(r_size->value_size);
				memcpy(o_size,     r_size->value,     r_size->value_size);
				if(data_bare_arithmetic(r_size->type,     o_size,     '*', data->linear_scale) != 0) return -EINVAL;
				r_size_str = r_size->key;
			}
			
			o_key_from = alloca(r_key_from->value_size);
			o_key_to   = alloca(r_key_to->value_size);
			
			memcpy(o_key_from, r_key_from->value, r_key_from->value_size);
			memcpy(o_key_to,   r_key_to->value,   r_key_to->value_size);
			
			if(data_bare_arithmetic(r_key_from->type, o_key_from, '*', data->linear_scale) != 0) return -EINVAL;
			if(data_bare_arithmetic(r_key_to->type,   o_key_to,   '*', data->linear_scale) != 0) return -EINVAL;
			
			hash_t  new_request[] = {
				{ r_key_from->key,  r_key_from->type, o_key_from, r_key_from->value_size  },
				{ r_key_to->key,    r_key_to->type,   o_key_to,   r_key_to->value_size    },
				{ r_size_str,       r_size->type    , o_size,     r_size->value_size      },
				hash_next(request)
			};
			
			ret = chain_next_query(chain, new_request, buffer);
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
