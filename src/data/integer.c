#include <libfrozen.h>

/* Integer - 8bit {{{1 */
int int8_cmp_func(void *data1, void *data2){
	int           cret;
	unsigned char data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned char *)data1;
	data2_val = *(unsigned char *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
}

REGISTER_DATA(TYPE_INT8,SIZE_FIXED,&int8_cmp_func,1)

/* }}}1 */

/* Integer - 16bit {{{1 */
int int16_cmp_func(void *data1, void *data2){
	int            cret;
	unsigned short data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned short *)data1;
	data2_val = *(unsigned short *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
}

REGISTER_DATA(TYPE_INT16,SIZE_FIXED,&int16_cmp_func,2)

/* }}}1 */

/* Integer - 32bit {{{1 */
int int32_cmp_func(void *data1, void *data2){
	int           cret;
	unsigned int  data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned int *)data1;
	data2_val = *(unsigned int *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
}

REGISTER_DATA(TYPE_INT32,SIZE_FIXED,&int32_cmp_func,4)

/* }}}1 */

/* Integer - 64bit {{{1 */
int int64_cmp_func(void *data1, void *data2){
	int                 cret;
	unsigned long long  data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned long long *)data1;
	data2_val = *(unsigned long long *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
}

REGISTER_DATA(TYPE_INT64,SIZE_FIXED,&int64_cmp_func,8)

/* }}}1 */

