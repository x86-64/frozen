#ifndef DATA_UNIQUE_T
#define DATA_UNIQUE_T

/** @ingroup data
 *  @addtogroup unique_t unique_t
 */
/** @ingroup unique_t
 *  @page unique_t_overview Overview
 *  
 *  This data wrap any PUSH/POP capable data to construct lists with unique items.
 */
/** @ingroup unique_t
 *  @page unique_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t unique = DATA_UNIQUET(data);
 *  @endcode
 */

#define DATA_UNIQUET(_data)  { TYPE_UNIQUET, (unique_t []){ { _data } }}
#define DEREF_TYPE_UNIQUET(_data) (unique_t *)((_data)->ptr)
#define REF_TYPE_UNIQUET(_dt) _dt
#define HAVEBUFF_TYPE_UNIQUET 0

typedef struct unique_t {
	data_t                 data;
} unique_t;

#endif
