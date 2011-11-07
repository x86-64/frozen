#line 1 "uint_init.m4"








#line 1 "uint.h.m4"


#ifndef DATA_UINT_INT64T_H
#define DATA_UINT_INT64T_H

/** @ingroup data
 *  @addtogroup int64_t int64_t
 */
/** @ingroup int64_t
 *  @page int64_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup int64_t
 *  @page int64_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_INT64T(100);
 *        
 *       int64_t some  = 200;
 *       data_t hpint = DATA_PTR_INT64T(&some);
 *  @endcode
 */

#define DATA_INT64T(value) { TYPE_INT64T, (int64_t []){ value } } 
#define DATA_PTR_INT64T(value) { TYPE_INT64T, value } 
#define DEREF_TYPE_INT64T(_data) *(int64_t *)((_data)->ptr) 
#define REF_TYPE_INT64T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_INT64T 1

#endif
/* vim: set filetype=m4: */
