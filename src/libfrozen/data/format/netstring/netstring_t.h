#ifndef DATA_NETSTRING_T_H
#define DATA_NETSTRING_T_H

/** @ingroup data
 *  @addtogroup netstring_t netstring_t
 */
/** @ingroup netstring_t
 *  @page netstring_t_overview Overview
 *  
 *  This data used to represent supplied data in netstring format.
 */
/** @ingroup netstring_t
 *  @page netstring_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t d_raw          = DATA_RAWT();
 *       data_t d_netstring;
 *
 *       data_netstring_t(&d_netstring, d_raw);
 *       // or
 *       data_t d_netstring    = DATA_NETSTRINGT(&d_raw);
 *  @endcode
 */

#include <core/void/void_t.h>

#define DATA_NETSTRINGT(_data)       { TYPE_NETSTRINGT, (netstring_t []){ { _data, DATA_VOID } }}

typedef struct netstring_t {
	data_t                *data;
	data_t                 storage;
} netstring_t;

API ssize_t data_netstring_t(data_t *data, data_t storage);

#endif
