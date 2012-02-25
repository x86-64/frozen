#ifndef DATA_BIND_T
#define DATA_BIND_T

/** @ingroup data
 *  @addtogroup bind_t bind_t
 */
/** @ingroup bind_t
 *  @page bind_t_overview Overview
 *  
 *  This data is wrapper - it works with master data in runtime, and dump info to slave on destroy action. On creation data from slave
 *  transfered to master data.
 */
/** @ingroup bind_t
 *  @page bind_t_define Define
 *
 *  Possible defines in C code:
 *  @code
 *       data_t bind = DATA_BINDT(_master,_slave,_format,_fatal,_sync);
 *  @endcode
 *
 *  In configuration file:
 *  @code
 *  {
 *        master        = (some_t)"",         // master data, would be used in runtime
 *        slave         = (some_t)"",         // slave data, used to save and load information
 *        format        = (format_t)"native",  // format in which data would be stored, default FORMAT(packed)
 *        fatal         = (uint_t)"1",        // do not ignore load errors, default 0
 *        sync          = (uint_t)"100"       // sync interval in number of actions, default 0 (no sync)
 *  }
 *  @endcode
 *
 *  @li Master data should support convert_to and convert_from with given format
 *  @li Slave should support basic read/write api, and, obviously, to write information on disk or on some storage device
 */

#define DATA_BINDT(_master,_slave,_format,_fatal,_sync)  { TYPE_BINDT, bind_t_alloc(_master,_slave,_format,_fatal) }
#define DEREF_TYPE_BINDT(_data) (bind_t *)((_data)->ptr)
#define REF_TYPE_BINDT(_dt) _dt
#define HAVEBUFF_TYPE_BINDT 0

typedef struct bind_t {
	data_t                 master;
	data_t                 slave;
	format_t               format;
	uintmax_t              sync;
	uintmax_t              sync_curr;
} bind_t;

bind_t *      bind_t_alloc(data_t *master, data_t *slave, format_t format, uintmax_t fatal, uintmax_t sync);

#endif
