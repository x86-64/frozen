#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_INT8T_H
#define DATA_UINT_INT8T_H

/** @ingroup data
 *  @addtogroup int8_t int8_t
 */
/** @ingroup int8_t
 *  @page int8_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup int8_t
 *  @page int8_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_INT8T(100);
 *        
 *       int8_t some  = 200;
 *       data_t hpint = DATA_PTR_INT8T(&some);
 *  @endcode
 */

#define DATA_INT8T(value) { TYPE_INT8T, (int8_t []){ value } } 
#define DATA_PTR_INT8T(value) { TYPE_INT8T, value } 
#define DEREF_TYPE_INT8T(_data) *(int8_t *)((_data)->ptr) 
#define REF_TYPE_INT8T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_INT8T 1

#endif
/* vim: set filetype=m4: */
