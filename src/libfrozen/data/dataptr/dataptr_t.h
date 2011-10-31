#ifndef DATA_DATAPTR_T_H
#define DATA_DATAPTR_T_H

/** @ingroup data
 *  @addtogroup dataptr_t dataptr_t
 */
/** @ingroup dataptr_t
 *  @page dataptr_t_overview Overview
 *  
 *  This data used to hold pointers to data. On read and writes it will return data holder content instead of actual data.
 */
/** @ingroup dataptr_t
 *  @page dataptr_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hptr = DATA_DATAPTRT(data);
 *  @endcode
 */

#define DATA_DATAPTRT(_data)         { TYPE_DATAPTRT, _data }
#define DEREF_TYPE_DATAPTRT(_data) (data_t *)((_data)->ptr)
#define REF_TYPE_DATAPTRT(_dt) _dt
#define HAVEBUFF_TYPE_DATAPTRT 0

#endif
