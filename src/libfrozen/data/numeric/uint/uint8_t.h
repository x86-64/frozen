#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_UINT8T_H
#define DATA_UINT_UINT8T_H

/** @ingroup data
 *  @addtogroup uint8_t uint8_t
 */
/** @ingroup uint8_t
 *  @page uint8_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup uint8_t
 *  @page uint8_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_UINT8T(100);
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_UINT8T(value) { TYPE_UINT8T,  (void *)(uintmax_t)(uint8_t)value } 
#define DATA_HEAP_UINT8T(value) { TYPE_UINT8T, data_uint8_t_alloc(value) } 
#define DEREF_TYPE_UINT8T(_data) (uint8_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_UINT8T(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_UINT8T(_dt) NULL 

#define HAVEBUFF_TYPE_UINT8T 1
#define UNVIEW_TYPE_UINT8T(_ret, _dt, _view)  {  _dt = *(uint8_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_UINT8T(value) { TYPE_UINT8T, (uint8_t []){ value } } 
#define DATA_HEAP_UINT8T(value) { TYPE_UINT8T, data_uint8_t_alloc(value) } 
#define DEREF_TYPE_UINT8T(_data) *(uint8_t *)((_data)->ptr) 
#define SET_TYPE_UINT8T(_data) *(uint8_t *)((_data)->ptr) 
#define REF_TYPE_UINT8T(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_UINT8T 1
#define UNVIEW_TYPE_UINT8T(_ret, _dt, _view)  {  _dt = *(uint8_t *)((_view)->ptr); _ret = 0; } 
#endif

uint8_t * data_uint8_t_alloc(uint8_t value);

#endif
/* vim: set filetype=m4: */
