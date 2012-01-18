#ifndef DATA_ALLOCATOR_LIST_T_H
#define DATA_ALLOCATOR_LIST_T_H

/** @ingroup data
 *  @addtogroup allocator_list_t allocator_list_t
 */
/** @ingroup allocator_list_t
 *  @page allocator_list_t_overview Overview
 *  
 * This data allocate and delete items from datatypes. Data kept in form of list.
 * Items can change their id's after delete actions.
 *
 * This data support PUSH/POP api set.
 */
/** @ingroup allocator_list_t
 *  @page allocator_list_t_define Configuration
 *
 * Accepted configuration:
 * @code
 * some = (allocator_list_t){
 *              item_size               = (uint_t)'10',       # specify size directly             OR
 *              item_sample             = (sometype_t)'',     # sample item for size estimation
 *
 * }
 * @endcode
 */

//#define DATA_ALLOCATOR_LISTT(...)      { TYPE_ALLOCATOR_LISTT, (allocator_list_t []){ __VA_ARGS__ } }
//#define DATA_PTR_ALLOCATOR_LISTT(...)  { TYPE_ALLOCATOR_LISTT, __VA_ARGS__ }
//#define DEREF_TYPE_ALLOCATOR_LISTT(_data) *(allocator_list_t *)((_data)->ptr)
//#define REF_TYPE_ALLOCATOR_LISTT(_dt) (&(_dt))
//#define HAVEBUFF_TYPE_ALLOCATOR_LISTT 1

#endif
