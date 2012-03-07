#ifndef DATA_NETSTRING_T_H
#define DATA_NETSTRING_T_H

/** @ingroup data
 *  @addtogroup netstring_t netstring_t
 */
/** @ingroup netstring_t
 *  @page netstring_t_overview Overview
 *  
 *  This data used to represent supplied data in netstring format.
 */
/** @ingroup netstring_t
 *  @page netstring_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hnetstring = DATA_NETSTRINGT(data)
 *  @endcode
 */

#include <core/void/void_t.h>

#define DATA_NETSTRINGT(_data)       { TYPE_NETSTRINGT, (netstring_t []){ { _data, DATA_VOID } }}
#define DATA_HEAP_NETSTRINGT(_size)  { TYPE_NETSTRINGT, netstring_t_alloc(_size)                   }
#define DEREF_TYPE_NETSTRINGT(_data) (netstring_t *)((_data)->ptr)
#define REF_TYPE_NETSTRINGT(_dt) _dt
#define HAVEBUFF_TYPE_NETSTRINGT 0

typedef struct netstring_t {
	data_t                *data;
	data_t                 freeit;
} netstring_t;

netstring_t *          netstring_t_alloc             (data_t *data);

#endif
