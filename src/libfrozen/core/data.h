#ifndef DATA_H
#define DATA_H

/** @file data.h */

/** @ingroup data
 *  @page data_overview Data overview
 *  
 *  Frozen tries to use native C data representation where it possible. Native types is
 *  uintmax_t (TYPE_UINTT), char * (TYPE_STRING), own structures such as hash_t, machine_t, datatype_t and so on.
 *
 *  To keep data type consistensy and generalize data processing "data holder" was introduced. This is
 *  simple structure data_t which consist of data type field and pointer to data. Data api accept only
 *  data wrapped in such holder. Wrapping can be achieved directly in user function, without unnessesery calls, so
 *  this have low overhead.
 *
 *  Data api, as machine api, receive data holder and request with specified action inside. If this data have
 *  correct handlers for action api performs it. There are many standard actions defined, for example, ACTION_READ, _WRITE,
 *  _COPY, _ALLOC, _FREE, _COMPARE, _CONVERT_TO, _CONVERT_FROM and so on.
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
 *  To use this data type in your data handlers, you need to supply several helper routines. This includes ACTION_PHYSICALLEN,
 *  ACTION_LOGICALLEN and ACTION_GETDATAPTR. They describe properties of memory chunk you want to process. Default_t itself
 *  know nothing about data it process. You also can't create clean default_t data, routines will return errors.
 */
/** @ingroup data
 *  @page data_memorymanagment Memory managment
 *  
 *  Almost every data can exist in three variations: within data section, within stack and within heap. Data api itself should not
 *  resist any of this representation. However, it is worth to mention, than some of data actions can produce only
 *  allocated data. This includes: ACTION_COPY, ACTION_ALLOC, ACTION_CONVERT_FROM and can be more. After calling this actions user
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
 *  Some of frozen api, for example hash_data_copy, can convert values from hash to desired type. It possible if data represent
 *  plain value (like integers), or plain pointer (as machine). For rest of data types, which require allocation, this process
 *  can't be automated, as requires free of allocated resources. For those data types you should manually call data_convert macro, or
 *  plain data_query with proper request.
 *
 *  All integer data can be converted in any other integer type. Buffer or string, containing human representation of integer, can also
 *  be converted in clean integer type. String with machine name can be converted to this machine pointer, hash with machine config
 *  can also be converted to new machine, and so on.
 *
 *  This ability also useful to avoid unnesessery data copying from place to place. For example, ipc_shmem machine use shared memory to
 *  send data to another process. If there was no conversion, it would require buffer with already packed data, which would be copied to
 *  shared memory. And, of course, special packing machine. Instead of that, ipc_shmem take any input and convert it directly to shared memory.
 */

#define DEF_BUFFER_SIZE 1024

#include <enum/datatype/datatype_t.h>

typedef enum data_api_type {
	API_DEFAULT_HANDLER,
	API_HANDLERS
} data_api_type;

typedef ssize_t    (*f_data_func)  (data_t *, void *args);

/// Data holder
struct data_t {
	datatype_t             type; ///< Data type. One of TYPE_*. Use datatype_t to hold this value.
	void                  *ptr;  ///< Pointer to data
};

struct data_proto_t {
	char                  *type_str;
	datatype_t             type;
	data_api_type          api_type;
	
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
API ssize_t              data_query             (data_t *data, void *args);

/** Read value from holder to supplied buffer
 * @param _ret  Return value (ssize_t)
 * @param _type Destination data type: one of TYPE_*. Only constants allowed
 * @param _dt   Destination data. (uintmax_t for TYPE_UINTT, char * for TYPE_STRINGT, etc)
 * @param _src  Source data holder (data_t *)
 * @retval -EINVAL Invalid source, or convertation error
 * @retval 0       Operation successful
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
	fastcall_transfer _r_transfer = { { 3, ACTION_TRANSFER }, _dst };                \
	_ret = data_query(&__data_src, &_r_transfer);                                    \
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
	fastcall_convert_from _r_convert = { { 4, ACTION_CONVERT_FROM }, _src, FORMAT(clean) };  \
	_ret = data_query(&__data_dst, &_r_convert);                                     \
	_dst = DEREF_##_type(&__data_dst);                                               \
	(void)_ret;                                                                      \
};

#endif // DATA_H
