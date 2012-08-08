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
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_UINTT(value) { TYPE_UINTT,  (void *)(uintmax_t)(uintmax_t)value } 
#define DATA_HEAP_UINTT(value) { TYPE_UINTT, data_uint_t_alloc(value) } 
#define DEREF_TYPE_UINTT(_data) (uintmax_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_UINTT(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_UINTT(_dt) NULL 

#define HAVEBUFF_TYPE_UINTT 1
#define UNVIEW_TYPE_UINTT(_ret, _dt, _view)  {  _dt = *(uintmax_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_UINTT(value) { TYPE_UINTT, (uintmax_t []){ value } } 
#define DATA_HEAP_UINTT(value) { TYPE_UINTT, data_uint_t_alloc(value) } 
#define DEREF_TYPE_UINTT(_data) *(uintmax_t *)((_data)->ptr) 
#define SET_TYPE_UINTT(_data) *(uintmax_t *)((_data)->ptr) 
#define REF_TYPE_UINTT(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINTT 1
#define UNVIEW_TYPE_UINTT(_ret, _dt, _view)  {  _dt = *(uintmax_t *)((_view)->ptr); _ret = 0; } 
#endif

uintmax_t * data_uint_t_alloc(uintmax_t value);

#endif
/* vim: set filetype=m4: */
