#ifndef DATA_STRING_T_H
#define DATA_STRING_T_H

/** @ingroup data
 *  @addtogroup string_t string_t
 */
/** @ingroup string_t
 *  @page string_t_overview Overview
 *  
 *  This data used to hold C strings.
 */
/** @ingroup string_t
 *  @page string_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t str1 = DATA_STRING("hello");
 *       data_t str2 = DATA_STRING(str_ptr);
 *  @endcode
 */

#define DATA_STRING(value)          { TYPE_STRINGT, value }
#define DATA_PTR_STRING(value)      { TYPE_STRINGT, value }
#define DEREF_TYPE_STRINGT(_data) (char *)((_data)->ptr)
#define REF_TYPE_STRINGT(_dt) _dt
#define HAVEBUFF_TYPE_STRINGT 0

#endif
