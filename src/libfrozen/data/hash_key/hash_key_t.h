#ifndef DATA_HASHKEY_T_H
#define DATA_HASHKEY_T_H

/** @ingroup data
 *  @addtogroup hash_key_t hash_key_t
 */
/** @ingroup hash_key_t
 *  @page hash_key_t_overview Overview
 *  
 *  This data used to hold hash keys.
 */
/** @ingroup hash_key_t
 *  @page hash_key_t_define Define
 *
 *  Possible defines:
 *  @code
 *       data_t hkey = DATA_HASHKEYT( HK(key) );
 *  @endcode
 */

#define DATA_HASHKEYT(...)  { TYPE_HASHKEYT, (hash_key_t []){ __VA_ARGS__ } }
#define DEREF_TYPE_HASHKEYT(_data) *(hash_key_t *)((_data)->ptr)
#define REF_TYPE_HASHKEYT(_dt) (&(_dt))
#define HAVEBUFF_TYPE_HASHKEYT 1

#endif
