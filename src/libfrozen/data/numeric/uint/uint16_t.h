#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINT16T_H
#define DATA_UINT_UINT16T_H

/** @ingroup data
 *  @addtogroup uint16_t uint16_t
 */
/** @ingroup uint16_t
 *  @page uint16_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uint16_t
 *  @page uint16_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINT16T(100);
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_UINT16T(value) { TYPE_UINT16T,  (void *)(uintmax_t)(uint16_t)value } 
#define DATA_HEAP_UINT16T(value) { TYPE_UINT16T, data_uint16_t_alloc(value) } 
#define DEREF_TYPE_UINT16T(_data) (uint16_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_UINT16T(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_UINT16T(_dt) NULL 

#define HAVEBUFF_TYPE_UINT16T 1
#define UNVIEW_TYPE_UINT16T(_ret, _dt, _view)  {  _dt = *(uint16_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_UINT16T(value) { TYPE_UINT16T, (uint16_t []){ value } } 
#define DATA_HEAP_UINT16T(value) { TYPE_UINT16T, data_uint16_t_alloc(value) } 
#define DEREF_TYPE_UINT16T(_data) *(uint16_t *)((_data)->ptr) 
#define SET_TYPE_UINT16T(_data) *(uint16_t *)((_data)->ptr) 
#define REF_TYPE_UINT16T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINT16T 1
#define UNVIEW_TYPE_UINT16T(_ret, _dt, _view)  {  _dt = *(uint16_t *)((_view)->ptr); _ret = 0; } 
#endif

uint16_t * data_uint16_t_alloc(uint16_t value);

#endif
/* vim: set filetype=m4: */
