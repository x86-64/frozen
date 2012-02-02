#ifndef DATA_SLICE_T
#define DATA_SLICE_T

/** @ingroup data
 *  @addtogroup slice_t slice_t
 */
/** @ingroup slice_t
 *  @page slice_t_overview Overview
 *  
 *  This data type make slice from supplied data. You can define offset to make slice from and
 *  limit (optional) size.
 *
 *  This is wrapper, data not copied.
 */
/** @ingroup slice_t
 *  @page slice_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t slice = DATA_SLICET(data, offset, size);  // slice from offset <offset> and limited to <size>
 *       data_t slice = DATA_SLICET(data, 10, ~0);     // slice from offset 10 and unlimited size
 *       data_t slice = DATA_SLICET(data, 10, 20);     // slice from offset 10 and limited to 20 size
 *  @endcode
 */

#define DATA_SLICET(_data,_off,_size)       { TYPE_SLICET, (slice_t []){{ _data, _off, _size }} }
#define DATA_HEAP_SLICET(_data,_off,_size)  { TYPE_SLICET, slice_t_alloc(_data, _off, _size)    }
#define DEREF_TYPE_SLICET(_data) (slice_t *)((_data)->ptr)
#define REF_TYPE_SLICET(_dt) _dt
#define HAVEBUFF_TYPE_SLICET 0

typedef struct slice_t {
	data_t                *data;
	uintmax_t              off;
	uintmax_t              size;
} slice_t;

API slice_t *       slice_t_alloc               (data_t *data, uintmax_t offset, uintmax_t size);
API slice_t *       slice_t_copy                (slice_t *list);
API void            slice_t_free                (slice_t *list);

#endif
