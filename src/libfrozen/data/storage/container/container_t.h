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
 *       data_t hcontainer = DATA_CONTAINERT;
 *       data_t hcontainer = DATA_HEAP_CONTAINERT;
 *  @endcode
 */

#include <storage/list/list_t.h>

#define DATA_CONTAINERT()           { TYPE_CONTAINERT, (container_t []){ LIST_INITIALIZER } }
#define DATA_HEAP_CONTAINERT()      { TYPE_CONTAINERT, container_alloc()                }
#define DATA_PTR_CONTAINERT(_buff)  { TYPE_CONTAINERT, (void *)_buff                    }
#define DEREF_TYPE_CONTAINERT(_data) (container_t *)((_data)->ptr)
#define REF_TYPE_CONTAINERT(_dt) _dt
#define HAVEBUFF_TYPE_CONTAINERT 0

/** @file container.h */

/// container_t structure
typedef struct container_t {
	list_t                 chunks;        ///< Container chunks
} container_t;

container_t *   container_alloc               (void);

#endif
