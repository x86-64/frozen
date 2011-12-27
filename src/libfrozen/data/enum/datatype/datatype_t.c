#include <libfrozen.h>
#include <dataproto.h>
#include <datatype_t.h>
#include <enum/format/format_t.h>

#define DYNAMIC_START_ID 1000

typedef struct dynamic_dt_t {
	uintmax_t              id;
	char                  *name;
} dynamic_dt_t;

static uintmax_t               dynamic_dt_id     = DYNAMIC_START_ID;
static list                    dynamic_dt_list   = LIST_INITIALIZER; // list is bad, coz of incrementing id simplier structure must be used

ssize_t             datatype_register        (data_proto_t *proto){ // {{{
	dynamic_dt_t          *new_dt;
	
	if( proto->type_str == NULL)
		return -EINVAL;
	
	if( (new_dt = malloc(sizeof(dynamic_dt_t))) == NULL )
		return -ENOMEM;
	
	new_dt->name = proto->type_str;
	new_dt->id   = dynamic_dt_id++;
	list_add(&dynamic_dt_list, new_dt);
	
	// TODO add proto to main list
	
	return 0;
} // }}}
uintmax_t           datatype_getid           (char *name){ // {{{
	datatype_t             res               = TYPE_INVALID;
	dynamic_dt_t          *dt;
	void                  *iter              = NULL;
	
	list_rdlock(&dynamic_dt_list);
		while( (dt = list_iter_next(&dynamic_dt_list, &iter)) != NULL){
			if(strcasecmp(dt->name, name) == 0){
				res = dt->id;
				break;
			}
		}
	list_unlock(&dynamic_dt_list);
	return res;
} // }}}

static ssize_t data_datatype_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	datatype_t             type;
	char                   buffer[DEF_BUFFER_SIZE];
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(datatype_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			uintmax_t     i;
			data_proto_t *proto;
			
			fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) < 0)
				return -EINVAL;
			
			buffer[r_read.buffer_size] = '\0';
			
			// check static datatypes
			for(i=0; i<data_protos_size; i++){
				if( (proto = data_protos[i]) == NULL)
					continue;
				
				if(strcasecmp(proto->type_str, buffer) == 0){
					type = proto->type;
					goto found;
				}
			}
			
			// check dynamic datatypes
			if( (type = datatype_getid(buffer)) != TYPE_INVALID)
				goto found;
			
			return -EINVAL;
			
		found:
			*(datatype_t *)(dst->ptr) = type;
			return 0;

		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_datatype_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	datatype_t             type;
	dynamic_dt_t          *dt;
	char                  *string            = "INVALID";
	void                  *iter              = NULL;
	
	if(src->ptr == NULL)
		return -EINVAL;
	
	type = *(datatype_t *)(src->ptr);
	
	if(type != TYPE_INVALID && (unsigned)type < data_protos_size){
		string = data_protos[type]->type_str;
	}else{
		if(type >= DYNAMIC_START_ID && type <= dynamic_dt_id){
			list_rdlock(&dynamic_dt_list);
				while( (dt = list_iter_next(&dynamic_dt_list, &iter)) != NULL){
					if(type == dt->id){
						string = dt->name;
						break;
					}
				}
			list_unlock(&dynamic_dt_list);
		}
	}
	
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, string, strlen(string) };
	ret        = data_query(fargs->dest, &r_write);
	
	if(fargs->header.nargs >= 5)
		fargs->transfered = r_write.buffer_size;
	
	return ret;
} // }}}		
static ssize_t data_datatype_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(datatype_t);
	return 0;
} // }}}

data_proto_t datatype_t_proto = {
	.type                   = TYPE_DATATYPET,
	.type_str               = "datatype_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_datatype_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_datatype_t_convert_from,
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_datatype_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_datatype_t_len,
	}
};
