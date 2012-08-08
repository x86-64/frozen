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
 *  @endcode
 */

#define DATA_INT8T(value) { TYPE_INT8T, (int8_t []){ value } } 
#define DATA_HEAP_INT8T(value) { TYPE_INT8T, data_int8_t_alloc(value) } 
#define DEREF_TYPE_INT8T(_data) *(int8_t *)((_data)->ptr) 
#define REF_TYPE_INT8T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_INT8T 1
#define UNVIEW_TYPE_INT8T(_ret, _dt, _view)  {  _dt = *(int8_t *)((_view)->ptr); _ret = 0; } 

int8_t * data_int8_t_alloc(int8_t value);

#endif
/* vim: set filetype=m4: */
