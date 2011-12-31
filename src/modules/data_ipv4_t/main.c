#include <libfrozen.h>
#include <ipv4_t.h>

#include <enum/format/format_t.h>

static ssize_t ipv4_from_string(uint32_t *pip, char *string, uintmax_t string_size){ // {{{
	uint64_t               ip, ip_part;
	uintmax_t              i;
	uintmax_t              have_dots         = 0;
	char                  *p;
	
	for(i=0; i<string_size; i++){
		switch(string[i]){
			case '.': have_dots++; break;  
			case 'x':                         // hex numbers
			case 'X': break;
			default:
				if(string[i] >= '0' && string[i] <= '9')
					break;
				if(string[i] >= 'A' && string[i] <= 'F')
					break;
				if(string[i] >= 'a' && string[i] <= 'f')
					break;
				string_size = i;
				goto exit;
		}
	}
	
exit:
	if(have_dots > 0){
		if(have_dots != 3)
			return -EINVAL;
		
		for(i=1, ip=0, p=string; i<= 4; i++){
			ip_part   = (uint8_t)strtoull(p, &p, 0);
			ip      <<= 8;
			ip       += ip_part;
			p++;
		}
	}else{
		ip  = strtoull(string, NULL, 0);
	}
	*pip = (uint32_t)ip;
	return 0;
} // }}}
static ssize_t ipv4_to_string(uint32_t ip, char *string, size_t *string_size, format_t format){ // {{{
	ssize_t                ret;
	
	switch(format){
		case FORMAT(human):
		case FORMAT(ip_dotint):
			if( (ret = snprintf(string, *string_size,
				"%d.%d.%d.%d",
				((ip >> 24) & 0xFF),
				((ip >> 16) & 0xFF),
				((ip >>  8) & 0xFF),
				((ip      ) & 0xFF)
			)) > *string_size)
				return -EINVAL;
			
			*string_size = ret; 
			return 0;
		case FORMAT(ip_dothexint):
			if( (ret = snprintf(string, *string_size,
				"0x%x.0x%x.0x%x.0x%x",
				((ip >> 24) & 0xFF),
				((ip >> 16) & 0xFF),
				((ip >>  8) & 0xFF),
				((ip      ) & 0xFF)
			)) > *string_size)
				return -EINVAL;
			
			*string_size = ret; 
			return 0;
		case FORMAT(ip_dotoctint):
			if( (ret = snprintf(string, *string_size,
				"0%o.0%o.0%o.0%o",
				((ip >> 24) & 0xFF),
				((ip >> 16) & 0xFF),
				((ip >>  8) & 0xFF),
				((ip      ) & 0xFF)
			)) > *string_size)
				return -EINVAL;
			
			*string_size = ret; 
			return 0;
		case FORMAT(ip_int):
			if( (ret = snprintf(string, *string_size,
				"%d",
				ip
			)) > *string_size)
				return -EINVAL;
			
			*string_size = ret; 
			return 0;
		case FORMAT(ip_hexint):
			if( (ret = snprintf(string, *string_size,
				"0x%x",
				ip
			)) > *string_size)
				return -EINVAL;
			
			*string_size = ret; 
			return 0;
		
		default:
			break;
	}
	return -ENOSYS;
} // }}}

static ssize_t data_ipv4_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(ipv4_t);
	return 0;
} // }}}
static ssize_t data_ipv4_t_alloc(data_t *data, fastcall_alloc *fargs){ // {{{
	if( (data->ptr = malloc(sizeof(ipv4_t))) == NULL)
		return -ENOMEM;
	return 0;
} // }}}
static ssize_t data_ipv4_t_compare(data_t *data1, fastcall_compare *fargs){ // {{{
	ssize_t                cret;
	uint32_t               data1_val, data2_val;
	
	if(fargs->data2 == NULL || fargs->data2->type != TYPE_IPV4T || fargs->data2->ptr == NULL)
		return -EINVAL;
	
	data1_val = ((ipv4_t *)(data1->ptr))->ip;
	data2_val = ((ipv4_t *)(fargs->data2->ptr))->ip; 
	
	      if(data1_val == data2_val){ cret = 0;
	}else if(data1_val  < data2_val){ cret = 1;
	}else{                            cret = 2; }
	return cret;
} // }}}
static ssize_t data_ipv4_t_arith_no_arg(data_t *data, fastcall_arith_no_arg *fargs){ // {{{
	ssize_t                ret               = 0;
	ipv4_t                *fdata             = (ipv4_t *)data->ptr;
	uint32_t               fdata_val         = fdata->ip;
	
	switch(fargs->header.action){
		case ACTION_INCREMENT:
			if(fdata_val == __MAX(uint32_t))
				return -EOVERFLOW;
			fdata_val++;
			break;
		case ACTION_DECREMENT:
			if(fdata_val == __MIN(uint32_t))
				return -EOVERFLOW;
			fdata_val--;
			break;
		default:
			return -ENOSYS;
	}
	fdata->ip = fdata_val;
	return ret;
} // }}}
static ssize_t data_ipv4_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	ipv4_t                *fdata             = (ipv4_t *)src->ptr;
	char                   buffer[DEF_BUFFER_SIZE];
	uintmax_t              buffer_size       = sizeof(buffer);
	
	if(fargs->dest == NULL || fdata == NULL)
		return -EINVAL;
	
	switch( fargs->format ){
		case FORMAT(clean):;
		case FORMAT(binary):;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &fdata->ip, sizeof(fdata->ip) };
			ret        = data_query(fargs->dest, &r_write);
			transfered = r_write.buffer_size;
			break;
		
		default:
			if( (ret = ipv4_to_string(fdata->ip, buffer, &buffer_size, fargs->format)) != 0)
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
static ssize_t data_ipv4_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	ipv4_t                *fdata;
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if(data_ipv4_t_alloc(dst, NULL) != 0)
			return -ENOMEM;
	}
	
	fdata = (ipv4_t *)dst->ptr;
	switch( fargs->format ){
		case FORMAT(config):;
		case FORMAT(human):;
			fastcall_read r_read_str = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read_str) != 0)
				return -EFAULT;
			return ipv4_from_string(&fdata->ip, buffer, r_read_str.buffer_size);
		
		case FORMAT(clean):;
		case FORMAT(binary):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &fdata->ip, sizeof(fdata->ip) };
			return data_query(fargs->src, &r_read);
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_ipv4_t_is_null(data_t *data, fastcall_is_null *fargs){ // {{{
	ipv4_t                *fdata             = (ipv4_t *)data->ptr;
	
	fargs->is_null = ( fdata == NULL || fdata->ip == 0 ) ? 1 : 0;
	return 0;
} // }}}

data_proto_t ipv4_proto = {
	.type_str               = IPV4T_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_ALLOC]          = (f_data_func)&data_ipv4_t_alloc,
		[ACTION_PHYSICALLEN]    = (f_data_func)&data_ipv4_t_len,
		[ACTION_LOGICALLEN]     = (f_data_func)&data_ipv4_t_len,
		[ACTION_COMPARE]        = (f_data_func)&data_ipv4_t_compare,
		[ACTION_INCREMENT]      = (f_data_func)&data_ipv4_t_arith_no_arg,
		[ACTION_DECREMENT]      = (f_data_func)&data_ipv4_t_arith_no_arg,
		[ACTION_CONVERT_TO]     = (f_data_func)&data_ipv4_t_convert_to,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_ipv4_t_convert_from,
		[ACTION_IS_NULL]        = (f_data_func)&data_ipv4_t_is_null,
	}
};

int main(void){
	data_register(&ipv4_proto);
	return 0;
}
