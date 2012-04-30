#ifndef DATA_IMMORTAL_T
#define DATA_IMMORTAL_T

/** @ingroup data
 *  @addtogroup immortal_t immortal_t
 */
/** @ingroup immortal_t
 *  @page immortal_t_overview Overview
 *  
 *  This data prevent _FREE action on data
 */
/** @ingroup immortal_t
 *  @page immortal_t_define Define
 *
 *  Possible C defines:
 *  @code
 *       data_t  immortal = DATA_IMMORTALT(data);
 *  @endcode
 */

#define DATA_IMMORTALT(_data)    { TYPE_IMMORTALT, (immortal_t []){ { _data } } }

typedef struct immortal_t {
	data_t                *data;
} immortal_t;

#endif
