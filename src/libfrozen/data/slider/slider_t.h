#ifndef DATA_SLIDER_T
#define DATA_SLIDER_T

/** @ingroup data
 *  @addtogroup slider_t slider_t
 */
/** @ingroup slider_t
 *  @page slider_t_overview Overview
 *  
 *  This can be used for continious read or write without calculating valid offset. Just wrap
 *  desired data and read (or write) from 0 offset every time.
 */
/** @ingroup slider_t
 *  @page slider_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t slider = DATA_SLIDERT(data, starting_offset);
 *  @endcode
 */

#define DATA_SLIDERT(_data,_off)  {TYPE_SLIDERT, (slider_t []){ { _data, _off } }}
#define DEREF_TYPE_SLIDERT(_data) (slider_t *)((_data)->ptr)
#define REF_TYPE_SLIDERT(_dt) _dt
#define HAVEBUFF_TYPE_SLIDERT 0

typedef struct slider_t {
	data_t                *data;
	uintmax_t              off;
} slider_t;

#endif
