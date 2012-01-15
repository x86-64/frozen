#ifndef DATA_NAMED_T_H
#define DATA_NAMED_T_H

/** @ingroup data
 *  @addtogroup named_t named_t
 */
/** @ingroup named_t
 *  @page named_t_overview Overview
 *  
 *  This data assign names for specified data for further usage.
 */
/** @ingroup named_t
 *  @page named_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hnamed = DATA_NAMEDT(data);
 *  @endcode
 */
/** @ingroup named_t
 *  @page named_t_behavior Behavior
 *
 *  @li COPY: because of global nature of named_t data, not possible to have copies of one data under same name, so COPY
 *      action not really performed - we only increment internal ref counter for that, and return same data for every copy.
 *
 *  @li CONVERT_FROM: then converting from hash supplied data copied and used in further actions.
 *
 */

#define DATA_NAMEDT(_name,_data)  { TYPE_NAMEDT, named_new(_name, _data) }
#define DEREF_TYPE_NAMEDT(_data) (named_t *)((_data)->ptr)
#define REF_TYPE_NAMEDT(_dt) _dt
#define HAVEBUFF_TYPE_NAMEDT 0

typedef struct named_t {
	char                  *name;
	data_t                *data;
	uintmax_t              refs;
} named_t;

API named_t *        named_new(char *name, data_t *data);

#endif
