#ifndef DATA_ALLOCATOR_FIXED_T_H
#define DATA_ALLOCATOR_FIXED_T_H

/** @ingroup data
 *  @addtogroup allocator_fixed_t allocator_fixed_t
 */
/** @ingroup allocator_fixed_t
 *  @page allocator_fixed_t_overview Overview
 *  
 * This data allocate and delete items from datatypes.
 */
/** @ingroup allocator_fixed_t
 *  @page allocator_fixed_t_define Configuration
 *
 * Accepted configuration:
 * @code
 * some = (allocator_fixed_t){
 *              item_size               = (uint_t)'10',                            # specify size directly             OR
 *              item_sample             = (sometype_t)'',                          # sample item for size estimation
 *              removed_items           =                                          # removed items tracker, if not supplied - no tracking performed
 *                                        (allocator_fixed_t){                     # - track with another allocator_fixed_t 
 *                   item_sample = (uint_t)"0"
 *              }
 *                                        (list_t){ ... }                          # - track with in-memory list_t
 *
 * }
 * @endcode
 */
/** @ingroup allocator_fixed_t
 *  @page allocator_fixed_t_reqs Removed items tracker requirements
 *
 * @li Thread safety. Required.
 * @li Support ACTION_PUSH and ACTION_POP functions. Required.
 * @li Support ACTION_LENGTH. Optional, for accurate measurement of elements count.
 */

//#define DATA_ALLOCATOR_FIXEDT(...)      { TYPE_ALLOCATOR_FIXEDT, (allocator_fixed_t []){ __VA_ARGS__ } }
//#define DATA_PTR_ALLOCATOR_FIXEDT(...)  { TYPE_ALLOCATOR_FIXEDT, __VA_ARGS__ }
//#define DEREF_TYPE_ALLOCATOR_FIXEDT(_data) *(allocator_fixed_t *)((_data)->ptr)
//#define REF_TYPE_ALLOCATOR_FIXEDT(_dt) (&(_dt))
//#define HAVEBUFF_TYPE_ALLOCATOR_FIXEDT 1

#endif
