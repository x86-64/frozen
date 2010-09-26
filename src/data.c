#include <libfrozen.h>

int                  data_type_is_valid     (data_type type){
	if(type != -1 && (unsigned)type < data_protos_size){
		return 1;
	}
	return 0;
}

data_type            data_type_from_string  (char *string){
	int i;
	
	for(i=0; i<data_protos_size; i++){
		if(strcasecmp(data_protos[i].type_str, string) == 0)
			return data_protos[i].type;
	}
	
	return -1;
}

data_proto_t *       data_proto_from_type   (data_type type){
	if((unsigned)type >= data_protos_size)
		return 0;
	
	return &data_protos[type];
}

int                  data_cmp               (data_type type, data_t *data1, data_t *data2){
	if((unsigned)type >= data_protos_size)
		return 0;
	
	f_data_cmp  func_cmp = data_protos[type].func_cmp;
	if(func_cmp == NULL)
		return 0;
	
	return func_cmp(data1, data2);
}

size_t               data_len               (data_type type, data_t *data, size_t buffer_size){
	f_data_len   func_len;
	
	if((unsigned)type >= data_protos_size)
		return 0;
	
	switch(data_protos[type].size_type){
		case SIZE_FIXED:
			return data_protos[type].fixed_size;
			
		case SIZE_VARIABLE:
			func_len = data_protos[type].func_len;
			
			if(func_len == NULL || data == NULL)
				return 0;
			
			return func_len(data, buffer_size);
	}
	return 0;
}

size_type            data_size_type         (data_type type){
	if((unsigned)type >= data_protos_size)
		return 0;
	
	return data_protos[type].size_type;
}

