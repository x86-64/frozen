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
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_INT64T(value) { TYPE_INT64T,  (void *)(uintmax_t)(int64_t)value } 
#define DATA_HEAP_INT64T(value) { TYPE_INT64T, data_int64_t_alloc(value) } 
#define DEREF_TYPE_INT64T(_data) (int64_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_INT64T(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_INT64T(_dt) NULL 

#define HAVEBUFF_TYPE_INT64T 1
#define UNVIEW_TYPE_INT64T(_ret, _dt, _view)  {  _dt = *(int64_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_INT64T(value) { TYPE_INT64T, (int64_t []){ value } } 
#define DATA_HEAP_INT64T(value) { TYPE_INT64T, data_int64_t_alloc(value) } 
#define DEREF_TYPE_INT64T(_data) *(int64_t *)((_data)->ptr) 
#define SET_TYPE_INT64T(_data) *(int64_t *)((_data)->ptr) 
#define REF_TYPE_INT64T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_INT64T 1
#define UNVIEW_TYPE_INT64T(_ret, _dt, _view)  {  _dt = *(int64_t *)((_view)->ptr); _ret = 0; } 
#endif

int64_t * data_int64_t_alloc(int64_t value);

#endif
/* vim: set filetype=m4: */
