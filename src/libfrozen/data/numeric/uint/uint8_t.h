#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINT8T_H
#define DATA_UINT_UINT8T_H

/** @ingroup data
 *  @addtogroup uint8_t uint8_t
 */
/** @ingroup uint8_t
 *  @page uint8_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uint8_t
 *  @page uint8_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINT8T(100);
 *        
 *       uint8_t some  = 200;
 *       data_t hpint = DATA_PTR_UINT8T(&some);
 *  @endcode
 */

#define DATA_UINT8T(value) { TYPE_UINT8T, (uint8_t []){ value } } 
#define DATA_PTR_UINT8T(value) { TYPE_UINT8T, value } 
#define DEREF_TYPE_UINT8T(_data) *(uint8_t *)((_data)->ptr) 
#define REF_TYPE_UINT8T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINT8T 1

#endif
/* vim: set filetype=m4: */
