#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINT64T_H
#define DATA_UINT_UINT64T_H

/** @ingroup data
 *  @addtogroup uint64_t uint64_t
 */
/** @ingroup uint64_t
 *  @page uint64_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uint64_t
 *  @page uint64_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINT64T(100);
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_UINT64T(value) { TYPE_UINT64T,  (void *)(uintmax_t)(uint64_t)value } 
#define DATA_HEAP_UINT64T(value) { TYPE_UINT64T, data_uint64_t_alloc(value) } 
#define DEREF_TYPE_UINT64T(_data) (uint64_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_UINT64T(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_UINT64T(_dt) NULL 

#define HAVEBUFF_TYPE_UINT64T 1
#define UNVIEW_TYPE_UINT64T(_ret, _dt, _view)  {  _dt = *(uint64_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_UINT64T(value) { TYPE_UINT64T, (uint64_t []){ value } } 
#define DATA_HEAP_UINT64T(value) { TYPE_UINT64T, data_uint64_t_alloc(value) } 
#define DEREF_TYPE_UINT64T(_data) *(uint64_t *)((_data)->ptr) 
#define SET_TYPE_UINT64T(_data) *(uint64_t *)((_data)->ptr) 
#define REF_TYPE_UINT64T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINT64T 1
#define UNVIEW_TYPE_UINT64T(_ret, _dt, _view)  {  _dt = *(uint64_t *)((_view)->ptr); _ret = 0; } 
#endif

uint64_t * data_uint64_t_alloc(uint64_t value);

#endif
/* vim: set filetype=m4: */
