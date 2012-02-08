#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINT64T_H
#define DATA_UINT_UINT64T_H

/** @ingroup data
 *  @addtogroup uint64_t uint64_t
 */
/** @ingroup uint64_t
 *  @page uint64_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uint64_t
 *  @page uint64_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINT64T(100);
 *        
 *       uint64_t some  = 200;
 *       data_t hpint = DATA_PTR_UINT64T(&some);
 *  @endcode
 */

#define DATA_UINT64T(value) { TYPE_UINT64T, (uint64_t []){ value } } 
#define DATA_PTR_UINT64T(value) { TYPE_UINT64T, value } 
#define DEREF_TYPE_UINT64T(_data) *(uint64_t *)((_data)->ptr) 
#define REF_TYPE_UINT64T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINT64T 1

#endif
/* vim: set filetype=m4: */
