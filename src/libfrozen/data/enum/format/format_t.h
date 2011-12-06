#ifndef DATA_FORMAT_T_H
#define DATA_FORMAT_T_H

#include <enum/format/format.h>

/** @ingroup data
 *  @addtogroup format_t format_t
 */
/** @ingroup format_t
 *  @page format_t_overview Overview
 *  
 *  This data used to hold formats
 */
/** @ingroup format_t
 *  @page format_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hkey = DATA_FORMATT( FORMAT(clean) );
 *  @endcode
 */

#define DATA_FORMATT(...)  { TYPE_FORMATT, (format_t []){ __VA_ARGS__ } }
#define DATA_PTR_FORMATT(...)  { TYPE_FORMATT, __VA_ARGS__ }
#define DEREF_TYPE_FORMATT(_data) *(format_t *)((_data)->ptr)
#define REF_TYPE_FORMATT(_dt) (&(_dt))
#define HAVEBUFF_TYPE_FORMATT 1

#endif
