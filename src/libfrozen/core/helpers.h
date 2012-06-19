#ifndef HELPERS_H
#define HELPERS_H

/** @file helpers.h */

// Key helpers {{{
/** @ingroup helpers
 *  @addtogroup helpers_keys Helper functions to work with keys
 */
/** @ingroup helpers_keys
 *  @page helpers_keys_overview Overview
 *  
 *  This function set used to work with complex keys, represented as hashes.
 */

/** @ingroup helpers_keys
 *
 * This function used to get one token from complex key.
 * 
 * @param[in]  key     Complex key
 * @param[out] token   Output key token, should be void_t
 * @retval <0  Error occurred
 * @retval 0   Call successful
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
 *  @li Call data_free then token no longer need
*/
API ssize_t              helper_key_current     (data_t *key, data_t *token);
// }}}
// Handler helpers {{{
/** @ingroup helpers
 *  @addtogroup helpers_handlers Helper functions for datatype handlers
 */
/** @ingroup helpers_handlers
 *  @page helpers_handlers_overview Overview
 *  
 *  This function set used to simplify datatype code.
 */
typedef struct helper_control_data_t {
	uintmax_t              key;
	data_t                *data;
} helper_control_data_t;

/** @ingroup helpers_handlers
 *
 * This function is helper in _CONTROL handler of datatype.
 * Use this as following:
 * @code
 * static ssize_t data_somedata_t_control(data_t *data, fastcall_control *fargs){
 *        somedata_t            *fdata              = (somedata_t *)data->ptr;
 *        
 *        switch(fargs->function){
 *                case HK(data):;
 *                        helper_control_data_t r_internal[] = {
 *                                { HK(data),   fdata->storage },
 *                                { HK(slave),  fdata->slave   },
 *                                hash_end
 *                        };
 *                        return helper_control_data(data, fargs, r_internal);
 *                default:
 *                        break;
 *        }
 *        return data_default_control(data, fargs);
 * }
 * @endcode
 * 
 * @param[in]  data           Data to get query
 * @param[in]  fargs          Fastcall _CONTROL arguments
 * @param[in]  internal_ptrs  Hash with internal data holders and keys
 * @retval <0  Error occurred
 * @retval 0   Call successful
 *
 *  <h3> For developer </h3>
 *  <h3> For user </h3>
*/

API ssize_t              helper_control_data     (data_t *data, fastcall_control *fargs, helper_control_data_t *internal_ptrs);
// }}}

// Data query helpers {{{
ssize_t              helper_data_convert_from(data_t *data, data_t *src, uintmax_t format, uintmax_t *transfered);
ssize_t              helper_data_convert_to(data_t *data, data_t *dest, uintmax_t format, uintmax_t *transfered);
ssize_t              helper_data_free(data_t *data);
ssize_t              helper_data_control(data_t *data, uintmax_t function, data_t *key, data_t *value);
ssize_t              helper_data_consume(data_t *data, data_t *dest);
ssize_t              helper_data_resize(data_t *data, uintmax_t length);
ssize_t              helper_data_length(data_t *data, uintmax_t *length, uintmax_t format);
ssize_t              helper_data_read(data_t *data, uintmax_t offset, void *buffer, uintmax_t *buffer_size);
ssize_t              helper_data_write(data_t *data, uintmax_t offset, void *buffer, uintmax_t *buffer_size);
ssize_t              helper_data_add(data_t *data, data_t *data2);
ssize_t              helper_data_sub(data_t *data, data_t *data2);
ssize_t              helper_data_mul(data_t *data, data_t *data2);
ssize_t              helper_data_div(data_t *data, data_t *data2);
ssize_t              helper_data_inc(data_t *data);
ssize_t              helper_data_dec(data_t *data);
ssize_t              helper_data_compare(data_t *data, data_t *data2, uintmax_t *result);
ssize_t              helper_data_create(data_t *data, data_t *key, data_t *value);
ssize_t              helper_data_lookup(data_t *data, data_t *key, data_t *value);
ssize_t              helper_data_update(data_t *data, data_t *key, data_t *value);
ssize_t              helper_data_delete(data_t *data, data_t *key, data_t *value);
ssize_t              helper_data_push(data_t *data, data_t *value);
ssize_t              helper_data_pop(data_t *data, data_t *value);
ssize_t              helper_data_enum(data_t *data, data_t *dest);
ssize_t              helper_data_view(data_t *data, uintmax_t format, void **ptr, uintmax_t *length, data_t *freeit);

data_t               helper_data_from_string(datatype_t type, char *string);
data_t               helper_data_from_hash(datatype_t type, hash_t *hash);
// }}}

#endif
