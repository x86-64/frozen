#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINT32T_H
#define DATA_UINT_UINT32T_H

/** @ingroup data
 *  @addtogroup uint32_t uint32_t
 */
/** @ingroup uint32_t
 *  @page uint32_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uint32_t
 *  @page uint32_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINT32T(100);
 *        
 *       uint32_t some  = 200;
 *       data_t hpint = DATA_PTR_UINT32T(&some);
 *  @endcode
 */

#define DATA_UINT32T(value) { TYPE_UINT32T, (uint32_t []){ value } } 
#define DATA_PTR_UINT32T(value) { TYPE_UINT32T, value } 
#define DEREF_TYPE_UINT32T(_data) *(uint32_t *)((_data)->ptr) 
#define REF_TYPE_UINT32T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINT32T 1

#endif
/* vim: set filetype=m4: */
