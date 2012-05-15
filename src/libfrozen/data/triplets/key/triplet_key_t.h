#ifndef DATA_TRIPLETKEY_T_H
#define DATA_TRIPLETKEY_T_H

/** @ingroup data
 *  @addtogroup triplet_key_t triplet_key_t
 */
/** @ingroup triplet_key_t
 *  @page triplet_key_t_overview Overview
 *  
 *  This data used to change key in CRUD request using supplied 3rd party data. New key is value returned from this data
 *  in lookup request.
 */
/** @ingroup triplet_key_t
 *  @page triplet_key_t_overview Behavior
 *  
 *  @li CREATE: Item created in storage with empty key. Resulting key saved to slave data as value to original request key.
 *  @li LOOKUP, UPDATE: Key from request looked up in slave data, if found - request to storage data with value
 *  @li DELETE: Key from request looked up in slave data, if found - request delete in storage data with value and finally remove key from slave.
 *  @li ENUM: Slave data enumerated and for each item storage data with this key called.
 */

typedef struct triplet_key_t {
	data_t                 storage;
	data_t                 slave;
} triplet_key_t;

API ssize_t data_triplet_key_t(data_t *data, data_t storage, data_t slave);

#endif
