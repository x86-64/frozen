#ifndef DATA_VOID_T_H
#define DATA_VOID_T_H

/** @ingroup data
 *  @addtogroup void_t void_t
 */
/** @ingroup void_t
 *  @page void_t_overview Overview
 *  
 *  This data used to represent empty value.
 */
/** @ingroup void_t
 *  @page void_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hvoid = DATA_VOID;
 *  @endcode
 */

#define DATA_VOID      { TYPE_VOIDT, NULL }
#define DEREF_TYPE_VOIDT(_data) (void *)((_data)->ptr)
#define REF_TYPE_VOIDT(_dt) _dt
#define HAVEBUFF_TYPE_VOIDT 0

#endif
