#ifndef DATA_MUTEX_T
#define DATA_MUTEX_T

/** @file mutex_t.h */
/** @ingroup data
 *  @addtogroup mutex_t mutex_t
 */
/** @ingroup mutex_t
 *  @page mutex_t_overview Overview
 *  
 *  This data used to wrap data in thread-safe manner
 */
/** @ingroup mutex_t
 *  @page mutex_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hmutex = DATA_MUTEXT(data);
 *  @endcode
 */

#include <pthread.h>

#define DATA_MUTEXT(_data)       { TYPE_MUTEXT, (mutex_t []){ { PTHREAD_MUTEX_INITIALIZER, _data } }}
#define DATA_HEAP_MUTEXT(_data)  { TYPE_MUTEXT, mutex_t_alloc(_data)                                }
#define DEREF_TYPE_MUTEXT(_data) (mutex_t *)((_data)->ptr)
#define REF_TYPE_MUTEXT(_dt) _dt
#define HAVEBUFF_TYPE_MUTEXT 0

typedef struct mutex_t {
	pthread_mutex_t        mutex;
	data_t                *data;
	data_t                 freeit;
} mutex_t;

mutex_t *              mutex_t_alloc             (data_t *data);

#endif
