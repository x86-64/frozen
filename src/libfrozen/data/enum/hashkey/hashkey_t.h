#ifndef DATA_HASHKEY_T_H
#define DATA_HASHKEY_T_H

/** @ingroup data
 *  @addtogroup hashkey_t hashkey_t
 */
/** @ingroup hashkey_t
 *  @page hashkey_t_overview Overview
 *  
 *  This data used to hold hash keys.
 */
/** @ingroup hashkey_t
 *  @page hashkey_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hkey = DATA_HASHKEYT( HK(key) );
 *  @endcode
 */

#include <enum/hashkey/hashkeys.h>

#define DATA_HASHKEYT(...)  { TYPE_HASHKEYT, (hashkey_t []){ __VA_ARGS__ } }
#define DATA_PTR_HASHKEYT(...)  { TYPE_HASHKEYT, __VA_ARGS__ }
#define DEREF_TYPE_HASHKEYT(_data) *(hashkey_t *)((_data)->ptr)
#define REF_TYPE_HASHKEYT(_dt) (&(_dt))
#define HAVEBUFF_TYPE_HASHKEYT 1

#endif
