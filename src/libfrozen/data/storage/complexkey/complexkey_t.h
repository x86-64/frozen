#ifndef DATA_COMPLEXKEY_T_H
#define DATA_COMPLEXKEY_T_H

/** @ingroup data
 *  @addtogroup complexkey_t complexkey_t
 */
/** @ingroup complexkey_t
 *  @page complexkey_t_overview Overview
 *  
 *  This data make linked list of keys.
 */
/** @ingroup complexkey_t
 *  @page complexkey_t_define Define
 *
 *  Possible defines:
 *  @code
 *      data_t hcomplexkey;
 *
 *      data_complexkey_t(hcomplexkey);
 *
 *	data_t          d_complex_key_next  = DATA_COMPLEXKEY_NEXTT(d_offset, *key);
 *	data_t          d_complex_key_end   = DATA_COMPLEXKEY_ENDT(d_offset);
 *	fastcall_create r_create       = {
 *		{ 4, ACTION_CREATE },
 *		data_is_void(key) ? 
 *			&d_complex_key_end :
 *			&d_complex_key_next,
 *		value
 *	};
 *  @endcode
 */

/** @file complexkey.h */

#define DATA_COMPLEXKEY_END_UINTT(_value)    { TYPE_COMPLEXKEYENDUINTT, (void *)_value                        }
#define DATA_COMPLEXKEY_ENDT(_value)         { TYPE_COMPLEXKEYENDT,     (complexkey_end_t []){ { _value } }   }
#define DATA_COMPLEXKEY_NEXTT(_value, _next) { TYPE_COMPLEXKEYT,        (complexkey_t []){ { _value, _next } }}
#define DATA_COMPLEXKEYT(_value, _next)      { TYPE_COMPLEXKEYT,        (complexkey_t []){ { _value, _next } }}

/// complexkey_t structure
typedef struct complexkey_t {
	data_t                 value;
	data_t                 cnext;
} complexkey_t;

typedef struct complexkey_end_t {
	data_t                 value;
} complexkey_end_t;

API ssize_t data_complexkey_t          (data_t *data, data_t storage, data_t cnext);
API ssize_t data_complexkey_end_t      (data_t *data, data_t storage);
API ssize_t data_complexkey_end_uint_t (data_t *data, uintmax_t value);

#endif
