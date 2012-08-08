#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_INT16T_H
#define DATA_UINT_INT16T_H

/** @ingroup data
 *  @addtogroup int16_t int16_t
 */
/** @ingroup int16_t
 *  @page int16_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup int16_t
 *  @page int16_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_INT16T(100);
 *  @endcode
 */

#define DATA_INT16T(value) { TYPE_INT16T, (int16_t []){ value } } 
#define DATA_HEAP_INT16T(value) { TYPE_INT16T, data_int16_t_alloc(value) } 
#define DEREF_TYPE_INT16T(_data) *(int16_t *)((_data)->ptr) 
#define REF_TYPE_INT16T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_INT16T 1
#define UNVIEW_TYPE_INT16T(_ret, _dt, _view)  {  _dt = *(int16_t *)((_view)->ptr); _ret = 0; } 

int16_t * data_int16_t_alloc(int16_t value);

#endif
/* vim: set filetype=m4: */
