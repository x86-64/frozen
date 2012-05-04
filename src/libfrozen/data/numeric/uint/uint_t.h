#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINTT_H
#define DATA_UINT_UINTT_H

/** @ingroup data
 *  @addtogroup uintmax_t uintmax_t
 */
/** @ingroup uintmax_t
 *  @page uintmax_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uintmax_t
 *  @page uintmax_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINTT(100);
 *        
 *       uintmax_t some  = 200;
 *       data_t hpint = DATA_PTR_UINTT(&some);
 *  @endcode
 */

#define DATA_UINTT(value) { TYPE_UINTT, (uintmax_t []){ value } } 
#define DATA_HEAP_UINTT(value) { TYPE_UINTT, data_uint_t_alloc(value) } 
#define DATA_PTR_UINTT(value) { TYPE_UINTT, value } 
#define DEREF_TYPE_UINTT(_data) *(uintmax_t *)((_data)->ptr) 
#define REF_TYPE_UINTT(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINTT 1
#define UNVIEW_TYPE_UINTT(_ret, _dt, _view)  {  _dt = *(uintmax_t *)((_view)->ptr); _ret = 0; } 

uintmax_t * data_uint_t_alloc(uintmax_t value);

#endif
/* vim: set filetype=m4: */
