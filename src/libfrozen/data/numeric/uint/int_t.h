#line 1 "uint_init.m4"




 

















#line 1 "uint.h.m4"


#ifndef DATA_UINT_INTT_H
#define DATA_UINT_INTT_H

/** @ingroup data
 *  @addtogroup intmax_t intmax_t
 */
/** @ingroup intmax_t
 *  @page intmax_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup intmax_t
 *  @page intmax_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_INTT(100);
 *        
 *       intmax_t some  = 200;
 *       data_t hpint = DATA_PTR_INTT(&some);
 *  @endcode
 */

#define DATA_INTT(value) { TYPE_INTT, (intmax_t []){ value } } 
#define DATA_HEAP_INTT(value) { TYPE_INTT, data_int_t_alloc(value) } 
#define DATA_PTR_INTT(value) { TYPE_INTT, value } 
#define DEREF_TYPE_INTT(_data) *(intmax_t *)((_data)->ptr) 
#define REF_TYPE_INTT(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_INTT 1
#define UNVIEW_TYPE_INTT(_ret, _dt, _view)  {  _dt = *(intmax_t *)((_view)->ptr); _ret = 0; } 

intmax_t * data_int_t_alloc(intmax_t value);

#endif
/* vim: set filetype=m4: */
