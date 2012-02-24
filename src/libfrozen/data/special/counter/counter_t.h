#ifndef DATA_COUNTER_T
#define DATA_COUNTER_T

/** @ingroup data
 *  @addtogroup counter_t counter_t
 */
/** @ingroup counter_t
 *  @page counter_t_overview Overview
 *  
 *  This count each read request it receive and return current counter value to buffer. 
 */
/** @ingroup counter_t
 *  @page counter_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t counter = DATA_COUNTERT(starting_value);
 *  @endcode
 */

#define DATA_COUNTERT(_start)  { TYPE_COUNTERT, (counter_t []){ { _start } }}
#define DEREF_TYPE_COUNTERT(_data) (counter_t *)((_data)->ptr)
#define REF_TYPE_COUNTERT(_dt) _dt
#define HAVEBUFF_TYPE_COUNTERT 0

typedef struct counter_t {
	data_t                 counter;
} counter_t;

#endif
