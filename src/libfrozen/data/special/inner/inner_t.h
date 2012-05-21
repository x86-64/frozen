#ifndef DATA_INNER_T
#define DATA_INNER_T

/** @ingroup data
 *  @addtogroup inner_t inner_t
 */
/** @ingroup inner_t
 *  @page inner_t_overview Overview
 *  
 *  This data is find inner data from specified child
 */

typedef struct inner_t {
	data_t                 storage;
	data_t                 key;
	uintmax_t              control;
} inner_t;

#endif
