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
 *       data_t hraw = DATA_RAWT_EMPTY();
 *  @endcode
 */

#define DATA_RAW(_buf,_size)         { TYPE_RAWT, (raw_t []){ { _buf, _size, 0 }      }}
#define DATA_RAWT(_buf,_size,_flags) { TYPE_RAWT, (raw_t []){ { _buf, _size, _flags } }}
#define DATA_RAWT_EMPTY()            { TYPE_RAWT, NULL                                 }
#define DATA_HEAP_RAWT(_size)        { TYPE_RAWT, raw_t_alloc(_size)                   }
#define data_raw_t_empty(_data)      { (_data)->type = TYPE_RAWT; (_data)->ptr = NULL; }

#define DEREF_TYPE_RAWT(_data) (raw_t *)((_data)->ptr)
#define REF_TYPE_RAWT(_dt) _dt
#define HAVEBUFF_TYPE_RAWT 0

typedef enum raw_flags_t {
	RAW_RESIZEABLE = 1,
	RAW_ONECHUNK   = 2
} raw_flags_t;

typedef struct raw_t {
	void                  *ptr;
	uintmax_t              size;
	raw_flags_t            flags;
} raw_t;

raw_t *          raw_t_alloc             (uintmax_t size);

#endif
