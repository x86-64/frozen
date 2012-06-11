#ifndef DATA_CONTAINER_T_H
#define DATA_CONTAINER_T_H

/** @ingroup data
 *  @addtogroup container_t container_t
 */
/** @ingroup container_t
 *  @page container_t_overview Overview
 *  
 *  This data used to hold any data within itself and make calls to whole set of data as it was glued together.
 */
/** @ingroup container_t
 *  @page container_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hcontainer;
 *
 *       data_container_t(hcontainer);
 *  @endcode
 */

/** @file container.h */

/// container_t structure
typedef struct container_t {
	data_t                 storage;
} container_t;

API ssize_t data_container_t (data_t *data, data_t storage);

#endif
