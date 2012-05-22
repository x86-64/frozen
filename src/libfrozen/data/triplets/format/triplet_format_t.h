#ifndef DATA_TRIPLETFORMAT_T_H
#define DATA_TRIPLETFORMAT_T_H

/** @ingroup data
 *  @addtogroup triplet_format_t triplet_format_t
 */
/** @ingroup triplet_format_t
 *  @page triplet_format_t_overview Overview
 *  
 *  This data used to convert value in CRUD request using supplied 3rd party data wich represent format. 
 */

typedef struct triplet_format_t {
	data_t                 storage;
	data_t                 slave;
} triplet_format_t;

API ssize_t data_triplet_format_t(data_t *data, data_t storage, data_t slave);

#endif
