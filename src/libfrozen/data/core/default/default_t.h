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

ssize_t       data_default_read          (data_t *data, fastcall_read *fargs);
ssize_t       data_default_write         (data_t *data, fastcall_write *fargs);
ssize_t       data_default_compare       (data_t *data1, fastcall_compare *fargs);
ssize_t       data_default_view          (data_t *data, fastcall_view *fargs);
ssize_t       data_default_is_null       (data_t *data, fastcall_is_null *fargs);
ssize_t       data_default_convert_to    (data_t *src, fastcall_convert_to *fargs);
ssize_t       data_default_convert_from  (data_t *dest, fastcall_convert_from *fargs);
ssize_t       data_default_free          (data_t *data, fastcall_free *fargs);
ssize_t       data_default_control       (data_t *data, fastcall_control *fargs);
ssize_t       data_default_consume       (data_t *data, fastcall_consume *fargs);
ssize_t       data_default_enum          (data_t *data, fastcall_enum *fargs);

#endif
