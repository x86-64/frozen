#ifndef DATA_REF_T
#define DATA_REF_T

/** @ingroup data
 *  @addtogroup ref_t ref_t
 */
/** @ingroup ref_t
 *  @page ref_t_overview Overview
 *  
 *  This data count references to supplied data and hold data until all refs would be destroyed.
 */
/** @ingroup ref_t
 *  @page ref_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t  ref = DATA_REFT(data);
 *
 *       ref_t  *ref = ref_t_alloc(data);
 *       data_t dref = DATA_PTR_REFT(ref);
 *
 *       ref_t_acquire(ref);
 *       ref_t_destroy(ref);
 *  @endcode
 */

#define DATA_REFT(_data)    { TYPE_REFT, ref_t_alloc(_data) }}
#define DATA_PTR_REFT(_ref) { TYPE_REFT, _ref }
#define DEREF_TYPE_REFT(_data) (ref_t *)((_data)->ptr)
#define REF_TYPE_REFT(_dt) _dt
#define HAVEBUFF_TYPE_REFT 0

typedef struct ref_t {
	data_t                 data;
	uintmax_t              refs;
} ref_t;

ssize_t           data_ref_t(data_t *data, data_t value);
ref_t *           ref_t_alloc(data_t *data);
void              ref_t_acquire(ref_t *ref);
void              ref_t_destroy(ref_t *ref);

#endif
