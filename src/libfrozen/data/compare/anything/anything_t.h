#ifndef DATA_ANYTHING_T_H
#define DATA_ANYTHING_T_H

/** @ingroup data
 *  @addtogroup anything_t anything_t
 */
/** @ingroup anything_t
 *  @page anything_t_overview Overview
 *  
 *  This data used to represent any value.
 */
/** @ingroup anything_t
 *  @page anything_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hanything = DATA_ANYTHING;
 *  @endcode
 */

#define DATA_ANYTHING      { TYPE_ANYTHINGT, NULL }
#define DEREF_TYPE_ANYTHINGT(_data) (anything *)((_data)->ptr)
#define REF_TYPE_ANYTHINGT(_dt) _dt
#define HAVEBUFF_TYPE_ANYTHINGT 0

#endif
