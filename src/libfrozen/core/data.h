#ifndef DATA_H
#define DATA_H

/** @file data.h */

/** @ingroup data
 *  @page data_overview Data overview
 *  
 *  Frozen tries to use native C data representation where it possible. Native types is
 *  uintmax_t (TYPE_UINTT), char * (TYPE_STRING), own structures such as hash_t, machine_t, datatype_t and so on.
 *
 *  To keep data type consistency and generalize data processing "data holder" was introduced. This is
 *  simple structure data_t which consist of data type field and pointer to data. Data api accept only
 *  data wrapped in such holder. Wrapping can be achieved directly in user function, without unnecessary calls, so
 *  this have low overhead.
 *
 *  Data api, as machine api, receive data holder and request with specified action inside. If this data have
 *  correct handlers for action api performs it. There are many standard actions defined, for example, ACTION_READ, _WRITE,
 *  _FREE, _COMPARE, _CONVERT_TO, _CONVERT_FROM and so on.
 *
 *  Data api use same structures and actions as API_FAST, so it will be easy to pass request, arrived to machine, to
 *  data. See @ref api_fast
 */
/** @ingroup data
 *  @page data_default Default data type (default_t)
 *  
 *  This data type is special. Then you create new data type it can inherit some common functions. For example,
 *  data type TYPE_RAWT internally consist of simple structure with pointer to actual data. There is no reasons
 *  to copy-and-paste such code as _READ, _WRITE routines. Also, this problem can be solved with complex inheritance support,
 *  which in turn rise complexity of whole program and can introduce more problems in future than help. So, default_t was introduced.
 *  It consist of safe functions to work with memory chunks.
 *  
 *  To use this data type in your data handlers, you need to supply several helper routines. This includes ACTION_LENGTH,
 *  and ACTION_VIEW. They describe properties of memory chunk you want to process. Default_t itself
 *  know nothing about data it process. You also can't create clean default_t data, routines will return errors.
 */
/** @ingroup data
 *  @page data_memorymanagment Memory management
 *  
 *  Almost every data can exist in three variations: within data section, within stack and within heap. Data api itself should not
 *  resist any of this representation. However, it is worth to mention, than some of data actions can produce only
 *  allocated data. This includes: ACTION_CONVERT_FROM and can be more. After calling this actions user
 *  is responsible to free allocated data.
 *
 *  Policy of data action handler should be following:
 *  @li if user supply empty data handler (i.e. with NULL pointer to data) api should silently allocate data and return result
 *  @li if user supply non-empty data - try to write to it, if this fails - return error.
 *
 *  Data handler should be ready to any: sectioned, stacked and heaped data. Never realloc or free data if you not 100% sure it was
 *  allocated (keep flag or something).
 *
 *  Same policy applied to data created and processed by machines: machine responsible to free allocated data, supply valid or empty
 *  data for request.
 */
/** @ingroup data
 *  @page data_conversion Data conversion
 *  
 *  Some of frozen api, for example hash_data_get, can convert values from hash to desired type. It possible if data represent
 *  plain value (like integers), or plain pointer (as machine). For rest of data types, which require allocation, this process
 *  can't be automated, as requires free of allocated resources. For those data types you should manually call data_convert macro, or
 *  plain data_query with proper request.
 *
 *  All integer data can be converted in any other integer type. Buffer or string, containing human representation of integer, can also
 *  be converted in clean integer type. String with machine name can be converted to this machine pointer, hash with machine config
 *  can also be converted to new machine, and so on.
 */

#define DEF_BUFFER_SIZE 1024

#include <enum/action/action_t.h>
#include <enum/datatype/datatype_t.h>
#include <enum/format/format_t.h>

typedef enum data_api_type {
	API_HANDLERS
} data_api_type;

typedef enum data_properties {
	DATA_GREEDY     = 1,
	DATA_ENDPOINT   = 2,
	DATA_PROXY      = 4,
} data_properties;

typedef ssize_t    (*f_data_func)  (data_t *, void *args);

/// Data holder
struct data_t {
	datatype_t             type; ///< Data type. One of TYPE_*. Use datatype_t to hold this value.
	void                  *ptr;  ///< Pointer to data
};

struct data_proto_t {
	datatype_t             type;
	uint32_t               type_port;
	char                  *type_str;
	data_api_type          api_type;
	data_properties        properties;
	
	f_data_func            handler_default;
	f_data_func            handlers[ACTION_INVALID];
};

extern data_proto_t **         data_protos;
extern uintmax_t               data_protos_nitems;

ssize_t                  frozen_data_init(void);
void                     frozen_data_destroy(void);

/** Register dynamic data type. Not thread-safe, not safe to run on working system
 * @param proto Data prototype
 * @retval -ENOMEM Insufficient memory
 * @retval 0       Call successful
*/
API ssize_t              data_register          (data_proto_t *proto);

/** Call action on data
 * @param data Data to process
 * @param args One of fastcall_* structs, with parameters to data
 * @retval -ENOSYS No such function
 * @retval -EINVAL Invalid data passed, or arguments count
 * @retval <0      Another error related to data implementation
 * @retval 0       Call successful
*/
#define data_query(_data, _args) data_protos[ ((data_t *)(_data))->type ]->handlers[ ((fastcall_header *)(_args))->action ](_data, _args)
//API ssize_t              data_query             (data_t *data, void *args);

/** Helper routine to get continious in memory data view with for given format. It first try to get native pointers (if supplied by data), if not - 
 * it start converting this data to raw_t.
 *
 * Caller must free freeme data if conversion to raw_t was made. It is safe to free it in any way, without checking ret value.
 * @param data Data to get
 * @param format Format in which get flat view
 * @param freeme Data to free after call
 * @param ptr      Pointer to void *
 * @param ptr_size Pointer to uintmax_t
 * @retval <0 Conversion to raw_t failed
 * @retval 0       Call successful, no conversion was made
 * @retval 1       Call successful, data converted to raw_t (need free)
*/
API ssize_t              data_make_flat         (data_t *data, format_t format, data_t *freeme, void **ptr, uintmax_t *ptr_size);

#define data_is_greedy(_data) (data_protos[ ((data_t *)(_data))->type ]->properties & DATA_GREEDY) != 0 ///< Check is data greedy?

/** Get real data holder
 * @param      _ret    Return code (ssize_t)
 * @param      _data   Data (data_t *)
 * @param[out] _holder Real holder (data_t *)
 */
#define data_realholder(_ret, _data, _holder){                 \
	static fastcall_control _r_control = {                 \
		{ 5, ACTION_CONTROL },                         \
		HK(data),                                      \
		NULL,                                          \
		NULL                                           \
	};                                                     \
	_ret    = data_query(_data, &_r_control);              \
	_holder = _r_control.value;                            \
}

/** Free data
 * @param _data Data to be freed
 */
#define data_free(_data){                                      \
	static fastcall_free _r_free = { { 2, ACTION_FREE } }; \
	data_query(_data, &_r_free);                           \
}

/** Void data. Can be used after free or data consuming.
 * @param _src Data to be void
 */
#define data_set_void(_src){       \
	(_src)->type = TYPE_VOIDT; \
	(_src)->ptr  = NULL;       \
}

/** Read value from holder to supplied buffer
 * @param _ret  Return value (ssize_t)
 * @param _type Destination data type: one of TYPE_*. Only constants allowed
 * @param _dt   Destination data. (uintmax_t for TYPE_UINTT, char * for TYPE_STRINGT, etc)
 * @param _src  Source data holder (data_t *)
 * @retval -EINVAL Invalid source, or convertation error
 * @retval 0       Operation successful
 */
#define data_get(_ret,_type,_dt,_src){                                         \
	datatype_t       _datatype;                                            \
	data_t           _d_type    = DATA_PTR_DATATYPET(&_datatype);          \
	fastcall_control _r_control = {                                        \
		{ 5, ACTION_CONTROL },                                         \
		HK(datatype), NULL, &_d_type                                   \
	};                                                                     \
	if( (_src) != NULL ){                                                  \
		if(                                                            \
			data_query((_src), &_r_control) >= 0 &&                \
			_datatype == _type                                     \
		){                                                             \
			fastcall_view _r_view = {                              \
				{ 6, ACTION_VIEW },                            \
				FORMAT(native)                                 \
			};                                                     \
			if( (_ret = data_query((_src), &_r_view)) >= 0){       \
				UNVIEW_##_type(_ret, _dt, &_r_view);           \
				data_free(&_r_view.freeit);                    \
			}                                                      \
		}else{                                                         \
			if( HAVEBUFF_##_type == 1 ){                           \
				data_convert(_ret, _type, _dt, _src);          \
			}else{                                                 \
				_ret = -EINVAL;                                \
			}                                                      \
		}                                                              \
	}else{                                                                 \
		_ret = -EINVAL;                                                \
	}                                                                      \
	(void)_ret;                                                            \
}

/** Consume value from holder to supplied buffer
 * @param _ret  Return value (ssize_t)
 * @param _type Destination data type: one of TYPE_*. Only constants allowed
 * @param _dt   Destination data. (uintmax_t for TYPE_UINTT, char * for TYPE_STRINGT, etc)
 * @param _src  Source data holder (data_t *)
 * @retval -EINVAL Invalid source, or convertation error
 * @retval 0       Operation successful
 */
#define data_consume(_ret,_type,_dt,_src){                                     \
	data_t  _d_consumed;                                                   \
	data_t *_d_fix = &_d_consumed;                                         \
	holder_consume(_ret, _d_consumed, _src);                               \
	if(_ret >= 0){                                                         \
		data_get(_ret, _type, _dt, _d_fix);                            \
	}                                                                      \
	(void)_ret;                                                            \
}

/** Consume holder
 * @param _ret  Return value (ssize_t)
 * @param _dt   Destination data holder. (data_t)
 * @param _src  Source data holder (data_t *)
 * @retval -EINVAL Invalid source, or convertation error
 * @retval 0       Operation successful
 */
#define holder_consume(_ret,_dst,_src){                                        \
	if((_src) != NULL){                                                    \
		fastcall_consume _r_consume = { { 3, ACTION_CONSUME }, &_dst };\
		_ret = data_query((_src), &_r_consume);                        \
	}else{                                                                 \
		_ret = -EINVAL;                                                \
	}                                                                      \
	(void)_ret;                                                            \
}

/** Copy data in holder
 * @param _ret  Return value (ssize_t)
 * @param _dt   Destination data holder. (data_t *)
 * @param _src  Source data holder (data_t *)
 * @retval -EINVAL Invalid source, or convertation error
 * @retval 0       Operation successful
 */
#define holder_copy(_ret,_dt,_src){                                            \
	(_dt)->type = (_src)->type;                                    \
	(_dt)->ptr  = NULL;                                            \
	fastcall_convert_from _r_convert = { { 5, ACTION_CONVERT_FROM }, _src, FORMAT(native) };  \
	_ret = data_query((_dt), &_r_convert);                         \
	(void)_ret;                                                            \
}

/** Write value from holder to supplied buffer.
 * @param _ret  Return value (ssize_t)
 * @param _type Source data type: one of TYPE_*. Only constants allowed
 * @param _dt   Source data. (uintmax_t for TYPE_UINTT, char * for TYPE_STRINGT, etc)
 * @param _dst  Destination data holder (data_t *)
 * @retval -EINVAL Invalid source, or convertation error
 * @retval 0       Operation successful
 */
#define data_set(_ret,_type,_dt,_dst){                                                   \
	data_t __data_src = { _type, REF_##_type(_dt) };                                 \
	fastcall_convert_to _r_convert = { { 5, ACTION_CONVERT_TO }, _dst, FORMAT(native) }; \
	_ret = data_query(&__data_src, &_r_convert);                                    \
	(void)_ret;                                                                      \
}

/** Convert value from holder to buffer.
 * If destination data 
 * @param _ret   Return value (ssize_t)
 * @param _type  Destination data type. Only constants allowed.
 * @param _dst   Destination data.
 * @param _src   Source data holder.
 * @retval 0     Convertation successful.
 * @retval <0    Convertation failed.
 */
#define data_convert(_ret, _type, _dst, _src) {                                          \
	data_t __data_dst = { _type, REF_##_type(_dst) };                                \
	fastcall_convert_from _r_convert = { { 5, ACTION_CONVERT_FROM }, _src, FORMAT(native) };  \
	_ret = data_query(&__data_dst, &_r_convert);                                     \
	_dst = DEREF_##_type(&__data_dst);                                               \
	(void)_ret;                                                                      \
};

#endif // DATA_H
