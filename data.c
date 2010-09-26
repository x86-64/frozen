#include <memcachedb.h>
#include <data.h>

static unsigned long long log_10(unsigned long long x){
	unsigned long long res   = 0;
	unsigned long long power = 10;
	
	while (power <= x){
		++res;
		power *= 10;
	}
	return res;
}

unsigned long data_unpacked_len(int data_type, char *data, unsigned long data_len){ // data size in string format
	switch(data_type){
		case DATA_TYPE_U8:      return log_10((unsigned long long)*(unsigned char *)data) + 1;
		case DATA_TYPE_U32:     return log_10((unsigned long long)*(unsigned int *)data) + 1;
		case DATA_TYPE_U64:     return log_10((unsigned long long)*(unsigned long long *)data) + 1;
		case DATA_TYPE_STRING:	return strlen(data);
		case DATA_TYPE_BINARY:
		default:
			return *(unsigned int *)data;
	};	
}

unsigned long data_packed_len(int data_type, char *data, unsigned long data_len){ // data size in binary format
	unsigned long len;

	switch(data_type){
		case DATA_TYPE_U8:     return 1;
		case DATA_TYPE_U32:    return 4;
		case DATA_TYPE_U64:    return 8;
		case DATA_TYPE_STRING: return strnlen(data, data_len) + 1;
		case DATA_TYPE_BINARY: 
		default:
			               return data_len + 4;
	};
}

void data_pack(int data_type, char *string, char *data, unsigned long data_len){ // convert string to binary formatted data
	unsigned long long data_ll;

	switch(data_type){
		case DATA_TYPE_U8:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
			data_ll = strtoull(string, NULL, 10);
			memcpy(data, &data_ll, data_len);
			break;
		case DATA_TYPE_STRING:
			memcpy(data, string, data_len - 1);
			break;
		case DATA_TYPE_BINARY:
		default:
			*(unsigned int *)data = data_len - 4;
			memcpy(data + 4, string, data_len - 4);
			break;
	};
}

void data_unpack(int data_type, char *data, unsigned long data_len, char *string){ // convert binary formatted data to string
	switch(data_type){
		case DATA_TYPE_U8:  sprintf(string,   "%d",      *(unsigned char *)data); break;
		case DATA_TYPE_U32: sprintf(string,   "%d",       *(unsigned int *)data); break;
		case DATA_TYPE_U64: sprintf(string, "%lld", *(unsigned long long *)data); break;
		case DATA_TYPE_STRING:
			memcpy(string, data, data_len);
			break;
		case DATA_TYPE_BINARY:
		default:
			memcpy(string, data + 4, data_len);
			break;
	}
}

unsigned int data_cmp_binary(int data_type, char *data1, char *data2){
	int                 cret;
	unsigned long long  item1_data;
	unsigned long long  item2_data;
	unsigned int        item1_len;
	unsigned int        item2_len;
	
	switch(data_type){
		case DATA_TYPE_U8:
			item1_data = (unsigned long long)*(unsigned char *)data1;
			item2_data = (unsigned long long)*(unsigned char *)data2; 
			     if(item1_data == item2_data){ cret =  0; }
			else if(item1_data <  item2_data){ cret = -1; }
			else                             { cret =  1; }
			break;
		case DATA_TYPE_U32:
			item1_data = (unsigned long long)*(unsigned int *)data1;
			item2_data = (unsigned long long)*(unsigned int *)data2; 
			     if(item1_data == item2_data){ cret =  0; }
			else if(item1_data <  item2_data){ cret = -1; }
			else                             { cret =  1; }
			break;
		case DATA_TYPE_U64:
			item1_data = (unsigned long long)*(unsigned long long *)data1;
			item2_data = (unsigned long long)*(unsigned long long *)data2; 
			     if(item1_data == item2_data){ cret =  0; }
			else if(item1_data <  item2_data){ cret = -1; }
			else                             { cret =  1; }	
			break;
		case DATA_TYPE_STRING:
			
			item1_len = strlen(data1);
			item2_len = strlen(data2);
	 		     if(item1_len > item2_len){ cret =  1; }
			else if(item1_len < item2_len){ cret = -1; }
			else {
				cret = strcmp(data1, data2);
				if(cret > 0) cret =  1;
				if(cret < 0) cret = -1;
			}
			break;
		case DATA_TYPE_BINARY:
		default:
			item1_len = *(unsigned int *)data1;
			item2_len = *(unsigned int *)data2;
			     if(item1_len > item2_len){ cret =  1; }
			else if(item1_len < item2_len){ cret = -1; }
			else {
				cret = memcmp((char *)data1 + 4, (char *)data2 + 4, item1_len);
				if(cret > 0) cret =  1;
				if(cret < 0) cret = -1;			
			}
			break;
	};
	return cret;	
}

int data_typestring_to_int(char *type_string){
	if(strcmp(type_string, "u8") == 0) return DATA_TYPE_U8;
	if(strcmp(type_string, "u32") == 0) return DATA_TYPE_U32;
	if(strcmp(type_string, "u64") == 0) return DATA_TYPE_U64;
	if(strcmp(type_string, "string") == 0) return DATA_TYPE_STRING;
	return DATA_TYPE_BINARY;
}
