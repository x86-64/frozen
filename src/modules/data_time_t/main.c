#include <libfrozen.h>
#include <time_t.h>

#include <enum/format/format_t.h>

static char           *time_formats[] = {
	"%a, %d %b %Y %H:%M:%S %z",            // rfc 2822
	"%Y-%m-%dT%H:%M:%S",                   // rfc 3339
	"%d/%m/%Y %H:%M:%S",                   // common date representation
	"%d/%m/%Y %H:%M",                      // common date representation
	"%d/%m/%Y",                            // common date representation
	"%d.%m.%Y %H:%M:%S",                   // common date representation
	"%d.%m.%Y %H:%M",                      // common date representation
	"%d.%m.%Y",                            // common date representation
	"%s",                                  // UNIX timestamp
};

static ssize_t timestamp_from_string(time_t *ptime, char *string, uintmax_t string_size){ // {{{
	uintmax_t              i;
	struct tm              timetm            = { 0 };
	
	for(i=0; i<sizeof(time_formats)/sizeof(time_formats[0]); i++){
		if(strptime(string, time_formats[i], &timetm) != NULL){
			*ptime = mktime(&timetm);
			return 0;
		}
	}
	return -EINVAL;
} // }}}
static ssize_t timestamp_to_string(time_t time, char *string, size_t *string_size, format_t format){ // {{{
	ssize_t                ret;
	intmax_t               time_format       = -1;
	struct tm              timetm;
	
	if(gmtime_r(&time, &timetm) == NULL)
		return -EINVAL;
	
	switch(format){
		case FORMAT(human):
		case FORMAT(time_rfc2822):      if(time_format == -1) time_format = 0;
		case FORMAT(time_rfc3339):      if(time_format == -1) time_format = 1;
		case FORMAT(time_slash_dmyhms): if(time_format == -1) time_format = 2;
		case FORMAT(time_slash_dmyhm):  if(time_format == -1) time_format = 3;
		case FORMAT(time_slash_dmy):    if(time_format == -1) time_format = 4;
		case FORMAT(time_dot_dmyhms):   if(time_format == -1) time_format = 5;
		case FORMAT(time_dot_dmyhm):    if(time_format == -1) time_format = 6;
		case FORMAT(time_dot_dmy):      if(time_format == -1) time_format = 7;
		case FORMAT(time_unix):         if(time_format == -1) time_format = 8;
		default:
			if(time_format == -1)
				break;
			
			if( (ret = strftime(string, *string_size, time_formats[time_format], &timetm)) == 0)
				return -EFAULT;
			
			*string_size = ret;
			return 0;
	}
	return -ENOSYS;
} // }}}

static ssize_t data_timestamp_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(timestamp_t);
	return 0;
} // }}}
static ssize_t data_timestamp_t_alloc(data_t *data, fastcall_alloc *fargs){ // {{{
	if( (data->ptr = malloc(sizeof(timestamp_t))) == NULL)
		return -ENOMEM;
	return 0;
} // }}}
static ssize_t data_timestamp_t_compare(data_t *data1, fastcall_compare *fargs){ // {{{
	ssize_t                cret;
	time_t                 data1_val, data2_val;
	
	if(fargs->data2 == NULL || fargs->data2->type != TYPE_TIMESTAMPT || fargs->data2->ptr == NULL)
		return -EINVAL;
	
	data1_val = ((timestamp_t *)(data1->ptr))->time;
	data2_val = ((timestamp_t *)(fargs->data2->ptr))->time;
	
	      if(data1_val == data2_val){ cret = 0;
	}else if(data1_val  < data2_val){ cret = 1;
	}else{                            cret = 2; }
	fargs->result = cret;
	return 0;
} // }}}
static ssize_t data_timestamp_t_arith_no_arg(data_t *data, fastcall_arith_no_arg *fargs){ // {{{
	ssize_t                ret               = 0;
	timestamp_t           *fdata             = (timestamp_t *)data->ptr;
	time_t                 fdata_val         = fdata->time;
	
	switch(fargs->header.action){
		case ACTION_INCREMENT:
			if(fdata_val == __MAX(time_t))
				return -EOVERFLOW;
			fdata_val++;
			break;
		case ACTION_DECREMENT:
			if(fdata_val == __MIN(time_t))
				return -EOVERFLOW;
			fdata_val--;
			break;
		default:
			return -ENOSYS;
	}
	fdata->time = fdata_val;
	return ret;
} // }}}
static ssize_t data_timestamp_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	timestamp_t           *fdata             = (timestamp_t *)src->ptr;
	char                   buffer[DEF_BUFFER_SIZE];
	uintmax_t              buffer_size       = sizeof(buffer);
	
	if(fargs->dest == NULL || fdata == NULL)
		return -EINVAL;
	
	switch( fargs->format ){
		case FORMAT(clean):;
		case FORMAT(binary):;
			fdata->time = 0;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &fdata->time, sizeof(fdata->time) };
			ret        = data_query(fargs->dest, &r_write);
			transfered = r_write.buffer_size;
			break;
		
		default:
			if( (ret = timestamp_to_string(fdata->time, buffer, &buffer_size, fargs->format)) != 0)
				return ret;
			
			fastcall_write r_write2 = { { 5, ACTION_WRITE }, 0, &buffer, buffer_size };
			ret        = data_query(fargs->dest, &r_write2);
			transfered = r_write.buffer_size;
			break;
	}
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_timestamp_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	timestamp_t           *fdata;
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if(data_timestamp_t_alloc(dst, NULL) != 0)
			return -ENOMEM;
	}
	
	fdata = (timestamp_t *)dst->ptr;
	switch( fargs->format ){
		case FORMAT(config):;
		case FORMAT(human):;
			fastcall_read r_read_str = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read_str) != 0)
				return -EFAULT;
			return timestamp_from_string(&fdata->time, buffer, r_read_str.buffer_size);
		
		case FORMAT(clean):;
		case FORMAT(binary):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &fdata->time, sizeof(fdata->time) };
			return data_query(fargs->src, &r_read);
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_timestamp_t_is_null(data_t *data, fastcall_is_null *fargs){ // {{{
	timestamp_t                *fdata             = (timestamp_t *)data->ptr;
	
	fargs->is_null = ( fdata == NULL || fdata->time == 0 ) ? 1 : 0;
	return 0;
} // }}}

data_proto_t timestamp_proto = {
	.type_str               = TIMESTAMPT_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_ALLOC]          = (f_data_func)&data_timestamp_t_alloc,
		[ACTION_PHYSICALLEN]    = (f_data_func)&data_timestamp_t_len,
		[ACTION_LOGICALLEN]     = (f_data_func)&data_timestamp_t_len,
		[ACTION_COMPARE]        = (f_data_func)&data_timestamp_t_compare,
		[ACTION_INCREMENT]      = (f_data_func)&data_timestamp_t_arith_no_arg,
		[ACTION_DECREMENT]      = (f_data_func)&data_timestamp_t_arith_no_arg,
		[ACTION_CONVERT_TO]     = (f_data_func)&data_timestamp_t_convert_to,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_timestamp_t_convert_from,
		[ACTION_IS_NULL]        = (f_data_func)&data_timestamp_t_is_null,
	}
};

int main(void){
	data_register(&timestamp_proto);
	return 0;
}
