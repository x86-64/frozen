#ifndef DATA_BINSTRING_T_H
#define DATA_BINSTRING_T_H

/** @ingroup data
 *  @addtogroup binstring_t binstring_t
 */
/** @ingroup binstring_t
 *  @page binstring_t_overview Overview
 *  
 *  This data used to represent supplied data in <length><data> format.
 */
/** @ingroup binstring_t
 *  @page binstring_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hbinstring = DATA_BINSTRINGT(data)
 *  @endcode
 */

#include <core/void/void_t.h>

#define DATA_BINSTRINGT(_data)       { TYPE_BINSTRINGT, (binstring_t []){ { _data, DATA_VOID } }}
#define DEREF_TYPE_BINSTRINGT(_data) (binstring_t *)((_data)->ptr)
#define REF_TYPE_BINSTRINGT(_dt) _dt
#define HAVEBUFF_TYPE_BINSTRINGT 0

typedef struct binstring_t {
	data_t                *data;
	data_t                 storage;
} binstring_t;

API ssize_t data_binstring_t(data_t *data, data_t storage);

#endif
