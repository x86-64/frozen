#ifndef DATA_LENGTH_T
#define DATA_LENGTH_T

/** @ingroup data
 *  @addtogroup length_t length_t
 */
/** @ingroup length_t
 *  @page length_t_overview Overview
 *  
 *  This data is shortcut for length querying. Act like read-only uint_t, on each query length measurement performed.
 */
/** @ingroup length_t
 *  @page length_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t length = DATA_LENGTHT(data);
 *  @endcode
 *
 *  In configuration:
 *  @code
 *  {
 *       somevar = (length_t){
 *              data   = (env_t)"somekey"                // required
 *              format = (format_t)"human"               // optional, default "clean"
 *       }
 *  }
 *  @endcode
 */

#define DATA_LENGTHT(_data,_format)  { TYPE_LENGTHT, (length_t []){ { _data, _format } }}
#define DEREF_TYPE_LENGTHT(_data) (length_t *)((_data)->ptr)
#define REF_TYPE_LENGTHT(_dt) _dt
#define HAVEBUFF_TYPE_LENGTHT 0

typedef struct length_t {
	data_t                 data;
	format_t               format;
} length_t;

#endif
