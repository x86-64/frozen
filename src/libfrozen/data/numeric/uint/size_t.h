#line 1 "uint_init.m4"






















#line 1 "uint.h.m4"


#ifndef DATA_UINT_SIZET_H
#define DATA_UINT_SIZET_H

/** @ingroup data
 *  @addtogroup size_t size_t
 */
/** @ingroup size_t
 *  @page size_t_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup size_t
 *  @page size_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_SIZET(100);
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
#define DATA_SIZET(value) { TYPE_SIZET,  (void *)(uintmax_t)(size_t)value } 
#define DATA_HEAP_SIZET(value) { TYPE_SIZET, data_size_t_alloc(value) } 
#define DEREF_TYPE_SIZET(_data) (size_t)((uintmax_t)((_data)->ptr)) 
#define SET_TYPE_SIZET(_data) ((_data)->ptr) 

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
#define REF_TYPE_SIZET(_dt) NULL 

#define HAVEBUFF_TYPE_SIZET 1
#define UNVIEW_TYPE_SIZET(_ret, _dt, _view)  {  _dt = *(size_t *)((_view)->ptr); _ret = 0; } 
#else
#define DATA_SIZET(value) { TYPE_SIZET, (size_t []){ value } } 
#define DATA_HEAP_SIZET(value) { TYPE_SIZET, data_size_t_alloc(value) } 
#define DEREF_TYPE_SIZET(_data) *(size_t *)((_data)->ptr) 
#define SET_TYPE_SIZET(_data) *(size_t *)((_data)->ptr) 
#define REF_TYPE_SIZET(_dt) (&(_dt)) 
#define HAVEBUFF_TYPE_SIZET 1
#define UNVIEW_TYPE_SIZET(_ret, _dt, _view)  {  _dt = *(size_t *)((_view)->ptr); _ret = 0; } 
#endif

size_t * data_size_t_alloc(size_t value);

#endif
/* vim: set filetype=m4: */
