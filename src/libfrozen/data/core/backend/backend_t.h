#ifndef DATA_BACKEND_H
#define DATA_BACKEND_H

/** @ingroup data
 *  @addtogroup backend_t backend_t
 */
/** @ingroup backend_t
 *  @page backend_t_overview Overview
 *  
 *  This data used to hold backends as data.
 *
 *  To create backend_t use conversion from hash (as config parameters), from string (as backend name) or backend api.
 */
/** @ingroup backend_t
 *  @page backend_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t htype = DATA_BACKENDT(backend);
 *  @endcode
 */

#define DATA_BACKENDT(_backend) { TYPE_BACKENDT, _backend }
#define DEREF_TYPE_BACKENDT(_data) (backend_t *)((_data)->ptr)
#define REF_TYPE_BACKENDT(_dt) _dt
#define HAVEBUFF_TYPE_BACKENDT 1

#endif
