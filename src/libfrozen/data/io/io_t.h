#ifndef DATA_IO_T
#define DATA_IO_T

/** @file io_t.h */
/** @ingroup data
 *  @addtogroup io_t io_t
 */
/** @ingroup io_t
 *  @page io_t_overview Overview
 *  
 *  This data used to make data with custom io handlers
 */
/** @ingroup io_t
 *  @page io_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hio = DATA_IOT(userdata, callback_func);
 *  @endcode
 */

#define DATA_IOT(_ud, _func)  {TYPE_IOT, (io_t []){ { _ud, _func } }}
#define DEREF_TYPE_IOT(_data) (io_t *)((_data)->ptr)
#define REF_TYPE_IOT(_dt) _dt
#define HAVEBUFF_TYPE_IOT 0

typedef ssize_t (*f_io_func) (data_t *data, void *userdata, void *args); ///< Io_t callback

typedef struct io_t {
	void *                 ud;
	f_io_func              handler;
} io_t;

#endif
