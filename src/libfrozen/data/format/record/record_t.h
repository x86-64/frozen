#ifndef DATA_RECORD_T_H
#define DATA_RECORD_T_H

/** @ingroup data
 *  @addtogroup record_t record_t
 */
/** @ingroup record_t
 *  @page record_t_overview Overview
 *  
 *  This data used to represent supplied data in "<data><separator>" format
 */
/** @ingroup record_t
 *  @page record_t_define Define
 *
 *  Possible defines:
 *  @code
 *  @endcode
 */

#include <core/void/void_t.h>

#define DEREF_TYPE_RECORDT(_data) (record_t *)((_data)->ptr)
#define REF_TYPE_RECORDT(_dt) _dt
#define HAVEBUFF_TYPE_RECORDT 0

#endif
