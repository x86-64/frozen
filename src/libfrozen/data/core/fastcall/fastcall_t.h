#ifndef DATA_FASTCALL_H
#define DATA_FASTCALL_H

/** @ingroup data
 *  @addtogroup fastcall_t fastcall_t
 */
/** @ingroup fastcall_t
 *  @page fastcall_t_overview Overview
 *  
 *  This data used to hold fastcalls as data.
 */
/** @ingroup fastcall_t
 *  @page fastcall_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t d_fastcall = DATA_FASTCALLT(&ret, &fastcall);     // current fastcall
 *  @endcode
 */

#define DATA_FASTCALLT(_ret, _fastcall)      { TYPE_FASTCALLT, (fastcall_t []){ { _ret, _fastcall }  }
#define DEREF_TYPE_FASTCALLT(_data) (fastcall_t *)((_data)->ptr)
#define REF_TYPE_FASTCALLT(_dt) _dt
#define HAVEBUFF_TYPE_FASTCALLT 0
#define UNVIEW_TYPE_FASTCALLT(_ret, _dt, _view)  { _dt = (fastcall_t *)((_view)->ptr); _ret = 0; }

typedef struct fastcall_t {
	uintmax_t             *ret;
	fastcall_header       *hargs;
} fastcall_t;

#endif
