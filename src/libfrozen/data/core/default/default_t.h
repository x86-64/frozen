#ifndef DATA_DEFAULT_T_H
#define DATA_DEFAULT_T_H

/** @ingroup data
 *  @addtogroup default_t default_t
 */
/** @ingroup default_t
 *  @page default_t_overview Overview
 *  
 *  See @ref data_default
 */

ssize_t default_transfer(data_t *src, data_t *dest, uintmax_t read_offset, uintmax_t write_offset, uintmax_t size, uintmax_t *ptransfered);

#endif
