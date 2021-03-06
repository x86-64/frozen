#ifndef DATA_DATATYPE_T_H
#define DATA_DATATYPE_T_H

/** @ingroup data
 *  @addtogroup datatype_t datatype_t
 */
/** @ingroup datatype_t
 *  @page datatype_t_overview Overview
 *  
 *  This data used to hold type of data. Valid values are all TYPE_* constants.
 */
/** @ingroup datatype_t
 *  @page datatype_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t htype = DATA_DATATYPET(TYPE_UINTT);
 *  @endcode
 */

#include <enum/datatype/datatype.h>

#define DATA_DATATYPET(...)      { TYPE_DATATYPET, (datatype_t []){ __VA_ARGS__ } }
#define DATA_PTR_DATATYPET(...)  { TYPE_DATATYPET, __VA_ARGS__ }
#define DEREF_TYPE_DATATYPET(_data) *(datatype_t *)((_data)->ptr)
#define REF_TYPE_DATATYPET(_dt) (&(_dt))
#define HAVEBUFF_TYPE_DATATYPET 1
#define UNVIEW_TYPE_DATATYPET(_ret, _dt, _view)  { _dt = *(datatype_t *)((_view)->ptr); _ret = 0; }

API datatype_t     datatype_t_getid_byname(char *name, ssize_t *pret);

#endif
