#ifndef DATA_H
#define DATA_H

/** @file data.h */

/** @ingroup data
 *  @page data_overview Data overview
 *  
 *  
 */

#define DEF_BUFFER_SIZE 1024

typedef enum data_formats {
	FORMAT_BINARY,
	FORMAT_HUMANREADABLE
} data_formats;

/// Data holder
typedef struct data_t {
	uintmax_t              type; ///< Data type. One of TYPE_*. Use datatype_t to hold this value.
	void                  *ptr;  ///< Pointer to data
} data_t;

/** Call action on data
 * @param data Data to process
 * @param args One of fastcall_* structs, with parameters to data
 * @retval -ENOSYS No such function
 * @retval -EINVAL Invalid data passed, or arguments count
 * @retval <0      Another error related to data implementation
 * @retval 0       Call successfull
*/
API ssize_t              data_query             (data_t *data, void *args);

/** Read value from holder to supplied buffer
 * @param _ret  Return value (ssize_t)
 * @param _type Destination data type: one of TYPE_*. Only constants allowed
 * @param _dt   Destination data. (uintmax_t for TYPE_UINTT, char * for TYPE_STRINGT, etc)
 * @param _src  Source data holder (data_t *)
 * @retval -EINVAL Invalid source, or convertation error
 * @retval 0       Operation successfull
 */
#define data_get(_ret,_type,_dt,_src){                                         \
	if((_src) != NULL && (_src)->type == _type){                           \
		_dt  = DEREF_##_type(_src);                                    \
		_ret = 0;                                                      \
	}else{                                                                 \
		if( HAVEBUFF_##_type == 1 ){                                   \
			data_convert(_ret, _type, _dt, _src);                  \
		}else{                                                         \
			_ret = -EINVAL;                                        \
		}                                                              \
	}                                                                      \
	(void)_ret;                                                            \
}

/** Convert value from holder to buffer.
 * If destination data 
 * @param _ret   Return value (ssize_t)
 * @param _type  Destination data type. Only constants allowed.
 * @param _dst   Destination data.
 * @param _src   Source data holder.
 * @retval 0     Convertation successfull.
 * @retval <0    Convertation failed.
 */
#define data_convert(_ret, _type, _dst, _src) {                                          \
	data_t __data_dst = { _type, REF_##_type(_dst) };                                \
	fastcall_convert_from _r_convert = { { 3, ACTION_CONVERT_FROM }, _src };         \
	_ret = data_query(&__data_dst, &_r_convert);                                     \
	_dst = DEREF_##_type(&__data_dst);                                               \
	(void)_ret;                                                                      \
};

#endif // DATA_H
