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

#endif
