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
 *       data_t hcomplexkey;
 *
 *       data_complexkey_t(hcomplexkey);
 *  @endcode
 */

/** @file complexkey.h */

#define DATA_COMPLEXKEYT(_value, _next) { TYPE_COMPLEXKEYT, (complexkey_t []){ { _value, _next } }}

/// complexkey_t structure
typedef struct complexkey_t {
	data_t                 value;
	data_t                 cnext;
} complexkey_t;

API ssize_t data_complexkey_t (data_t *data, data_t storage, data_t cnext);

#endif
