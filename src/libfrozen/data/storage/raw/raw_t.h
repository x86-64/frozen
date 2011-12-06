#ifndef DATA_RAW_T_H
#define DATA_RAW_T_H

/** @ingroup data
 *  @addtogroup raw_t raw_t
 */
/** @ingroup raw_t
 *  @page raw_t_overview Overview
 *  
 *  This data used to hold any binary data.
 */
/** @ingroup raw_t
 *  @page raw_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hraw = DATA_RAW(buffer, buffer_size);
 *  @endcode
 */

#define DATA_RAW(_buf,_size) {TYPE_RAWT, (raw_t []){ { _buf, _size } }}
#define DEREF_TYPE_RAWT(_data) (raw_t *)((_data)->ptr)
#define REF_TYPE_RAWT(_dt) _dt
#define HAVEBUFF_TYPE_RAWT 0

typedef struct raw_t {
	void                  *ptr;
	uintmax_t              size;
} raw_t;

#endif
