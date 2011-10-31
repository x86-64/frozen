#ifndef DATA_BUFFER_T_H
#define DATA_BUFFER_T_H

/** @ingroup data
 *  @addtogroup buffer_t buffer_t
 */
/** @ingroup buffer_t
 *  @page buffer_t_overview Overview
 *  
 *  This data used to hold buffers. See @ref buffer.h for buffer api.
 */
/** @ingroup buffer_t
 *  @page buffer_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hbuffer = DATA_BUFFERT(buffer);
 *  @endcode
 */

#define DATA_BUFFERT(_buff)   { TYPE_BUFFERT, (void *)_buff }
#define DEREF_TYPE_BUFFERT(_data) (buffer_t *)((_data)->ptr)
#define REF_TYPE_BUFFERT(_dt) _dt
#define HAVEBUFF_TYPE_BUFFERT 0

#endif
