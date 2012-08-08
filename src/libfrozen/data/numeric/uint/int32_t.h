#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_INT32T_H
#define DATA_UINT_INT32T_H

/** @ingroup data
 *  @addtogroup int32_t int32_t
 */
/** @ingroup int32_t
 *  @page int32_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup int32_t
 *  @page int32_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_INT32T(100);
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_INT32T(value) { TYPE_INT32T,  (void *)(uintmax_t)(int32_t)value } 
#define DATA_HEAP_INT32T(value) { TYPE_INT32T, data_int32_t_alloc(value) } 
#define DEREF_TYPE_INT32T(_data) (int32_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_INT32T(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_INT32T(_dt) NULL 

#define HAVEBUFF_TYPE_INT32T 1
#define UNVIEW_TYPE_INT32T(_ret, _dt, _view)  {  _dt = *(int32_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_INT32T(value) { TYPE_INT32T, (int32_t []){ value } } 
#define DATA_HEAP_INT32T(value) { TYPE_INT32T, data_int32_t_alloc(value) } 
#define DEREF_TYPE_INT32T(_data) *(int32_t *)((_data)->ptr) 
#define SET_TYPE_INT32T(_data) *(int32_t *)((_data)->ptr) 
#define REF_TYPE_INT32T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_INT32T 1
#define UNVIEW_TYPE_INT32T(_ret, _dt, _view)  {  _dt = *(int32_t *)((_view)->ptr); _ret = 0; } 
#endif

int32_t * data_int32_t_alloc(int32_t value);

#endif
/* vim: set filetype=m4: */
