#ifndef DATA_DATA_T_H
#define DATA_DATA_T_H

/** @ingroup data
 *  @addtogroup data_t data_t
 */
/** @ingroup data_t
 *  @page data_t_overview Overview
 *  
 *  This data used to represent data_t.
 */
/** @ingroup data_t
 *  @page data_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hdata = DATA_PTR_DATAT(_data);
 *  @endcode
 */

#define DATA_PTR_DATAT(_data)      { TYPE_DATAT, _data }
#define DEREF_TYPE_DATAT(_data) (data *)((_data)->ptr)
#define REF_TYPE_DATAT(_dt) _dt
#define HAVEBUFF_TYPE_DATAT 0

#endif
