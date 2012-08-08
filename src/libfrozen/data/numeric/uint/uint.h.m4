m4_include(uint_init.m4)

[#ifndef DATA_UINT_]DEF()_H
[#define DATA_UINT_]DEF()_H

/** @ingroup data
 *  @addtogroup []TYPE() []TYPE()
 */
/** @ingroup []TYPE()
 *  @page []TYPE()_overview Overview
 *  
 *  This data used to hold integers of different size.
 */
/** @ingroup []TYPE()
 *  @page []TYPE()_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hint  = DATA_[]DEF()(100);
 *  @endcode
 */

#ifdef OPTIMIZE_UINT
[#define DATA_]DEF()[(value) { TYPE_]DEF()[,  (void *)(uintmax_t)(]TYPE()[)value } ]
[#define DATA_HEAP_]DEF()[(value) { TYPE_]DEF()[, data_]NAME()[_alloc(value) } ]
[#define DEREF_TYPE_]DEF()[(_data) (]TYPE()[)((uintmax_t)((_data)->ptr)) ]
[#define SET_TYPE_]DEF()[(_data) ((_data)->ptr) ]

// BUG won't work with data_set, used because of warnings on uninitialized variable in data_convert
[#define REF_TYPE_]DEF()[(_dt) NULL ]

[#define HAVEBUFF_TYPE_]DEF() 1
[#define UNVIEW_TYPE_]DEF()[(_ret, _dt, _view)  {  _dt = *(]TYPE()[ *)((_view)->ptr); _ret = 0; } ]
#else
[#define DATA_]DEF()[(value) { TYPE_]DEF()[, (]TYPE()[ []){ value } } ]
[#define DATA_HEAP_]DEF()[(value) { TYPE_]DEF()[, data_]NAME()[_alloc(value) } ]
[#define DEREF_TYPE_]DEF()[(_data) *(]TYPE()[ *)((_data)->ptr) ]
[#define SET_TYPE_]DEF()[(_data) *(]TYPE()[ *)((_data)->ptr) ]
[#define REF_TYPE_]DEF()[(_dt) (&(_dt)) ]
[#define HAVEBUFF_TYPE_]DEF() 1
[#define UNVIEW_TYPE_]DEF()[(_ret, _dt, _view)  {  _dt = *(]TYPE()[ *)((_view)->ptr); _ret = 0; } ]
#endif

TYPE() * data_[]NAME()_alloc([]TYPE() value);

[#endif]
/* vim: set filetype=m4: */
