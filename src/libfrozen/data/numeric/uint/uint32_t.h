#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINT32T_H
#define DATA_UINT_UINT32T_H

/** @ingroup data
 *  @addtogroup uint32_t uint32_t
 */
/** @ingroup uint32_t
 *  @page uint32_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uint32_t
 *  @page uint32_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINT32T(100);
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_UINT32T(value) { TYPE_UINT32T,  (void *)(uintmax_t)(uint32_t)value } 
#define DATA_HEAP_UINT32T(value) { TYPE_UINT32T, data_uint32_t_alloc(value) } 
#define DEREF_TYPE_UINT32T(_data) (uint32_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_UINT32T(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_UINT32T(_dt) NULL 

#define HAVEBUFF_TYPE_UINT32T 1
#define UNVIEW_TYPE_UINT32T(_ret, _dt, _view)  {  _dt = *(uint32_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_UINT32T(value) { TYPE_UINT32T, (uint32_t []){ value } } 
#define DATA_HEAP_UINT32T(value) { TYPE_UINT32T, data_uint32_t_alloc(value) } 
#define DEREF_TYPE_UINT32T(_data) *(uint32_t *)((_data)->ptr) 
#define SET_TYPE_UINT32T(_data) *(uint32_t *)((_data)->ptr) 
#define REF_TYPE_UINT32T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINT32T 1
#define UNVIEW_TYPE_UINT32T(_ret, _dt, _view)  {  _dt = *(uint32_t *)((_view)->ptr); _ret = 0; } 
#endif

uint32_t * data_uint32_t_alloc(uint32_t value);

#endif
/* vim: set filetype=m4: */
