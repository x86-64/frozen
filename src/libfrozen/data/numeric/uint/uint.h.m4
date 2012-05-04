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
 *        
 *       TYPE() some  = 200;
 *       data_t hpint = DATA_PTR_[]DEF()(&some);
 *  @endcode
 */

[#define DATA_]DEF()[(value) { TYPE_]DEF()[, (]TYPE()[ []){ value } } ]
[#define DATA_HEAP_]DEF()[(value) { TYPE_]DEF()[, data_]NAME()[_alloc(value) } ]
[#define DATA_PTR_]DEF()[(value) { TYPE_]DEF()[, value } ]
[#define DEREF_TYPE_]DEF()[(_data) *(]TYPE()[ *)((_data)->ptr) ]
[#define REF_TYPE_]DEF()[(_dt) (&(_dt)) ]
[#define HAVEBUFF_TYPE_]DEF() 1
[#define UNVIEW_TYPE_]DEF()[(_ret, _dt, _view)  {  _dt = *(]TYPE()[ *)((_view)->ptr); _ret = 0; } ]

TYPE() * data_[]NAME()_alloc([]TYPE() value);

[#endif]
/* vim: set filetype=m4: */
