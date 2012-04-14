#ifndef DATA_TIMET_H
#define DATA_TIMET_H

/** @ingroup data
 *  @addtogroup timestamp_t timestamp_t
 */
/** @ingroup timestamp_t
 *  @page timestamp_t_overview Overview
 *  
 *  This data used to hold timestamps.
 */

#define TIMESTAMPT_NAME  "timestamp_t"
#define TYPE_TIMESTAMPT  datatype_t_getid_byname(TIMESTAMPT_NAME, NULL)

#define DATA_TIMESTAMPT(value) { TYPE_TIMESTAMPT, (timestamp_t []){ value } } 
#define DATA_PTR_TIMESTAMPT(value) { TYPE_TIMESTAMPT, value } 
#define DEREF_TYPE_TIMESTAMPT(_data) (timestamp_t *)((_data)->ptr) 
#define REF_TYPE_TIMESTAMPT(_dt) _dt 
#define HAVEBUFF_TYPE_TIMESTAMPT 0

typedef struct timestamp_t {
	time_t                 time;
	uintmax_t              now;
} timestamp_t;

#endif
