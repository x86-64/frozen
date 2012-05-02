#ifndef DATA_CONSUMABLE_T
#define DATA_CONSUMABLE_T

/** @ingroup data
 *  @addtogroup consumable_t consumable_t
 */
/** @ingroup consumable_t
 *  @page consumable_t_overview Overview
 *  
 *  This datatype is wrapper to show that wrapped data is ready to be consumed as is, without copying. 
 */
/** @ingroup consumable_t
 *  @page consumable_t_define Define
 *
 *  Possible C defines:
 *  @code
 *       data_t  consumable    = DATA_CONSUMABLET(data);
 *       data_t  notconsumable = DATA_NOTCONSUMABLET(data);
 *
 *       data_t consumable;
 *       data_consumable_t(&consumable, child);
 *
 *       data_t notconsumable;
 *       data_notconsumable_t(&notconsumable, child);
 *  @endcode
 */

#define DATA_CONSUMABLET(_data)         { TYPE_CONSUMABLET, (consumable_t []){ { _data, 0, 0 } } }
#define DATA_NOTCONSUMABLET(_data)      { TYPE_CONSUMABLET, (consumable_t []){ { _data, 1, 0 } } }

typedef struct consumable_t {
	data_t                 data;
	uintmax_t              not_consumable;
	uintmax_t              heap;
} consumable_t;

API ssize_t             data_consumable_t   (data_t *data, data_t child);
API ssize_t             data_notconsumable_t(data_t *data, data_t child);

#endif
