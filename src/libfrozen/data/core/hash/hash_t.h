#ifndef DATA_HASH_T_H
#define DATA_HASH_T_H

#include <enum/hashkey/hashkey_t.h>

/** @ingroup hash_t
 *  @page hash_t_define Define
 *
 *  Possible defines:
 *  @code
 *      hash_t  newhash1[] = {                              // actual hash
 *          { HK(key),   DATA_UINTT(100) },
 *          hash_end
 *      };
 *      data_t  hnewhash1  = DATA_PTR_HASHT(&newhash1);     // data holder
 *      
 *      data_t  hnewhash2  = DATA_HASHT(                    // only data holder
 *           { HK(key),  DATA_UINTT(200) },
 *           hash_end
 *      );
 *
 *      hash_t  somehash[] = {                              // as key in another hash
 *           { HK(hash), DATA_HASHT(
 *                  { HK(key),  DATA_UINTT(300) },
 *                  hash_end
 *           )},
 *           hash_end
 *      };
 *  @endcode
 */

#define DATA_HASHT(...)             { TYPE_HASHT, (hash_t []){ __VA_ARGS__ } }
#define DATA_PTR_HASHT(_hash)       { TYPE_HASHT, (void *)_hash }
#define DEREF_TYPE_HASHT(_data) (hash_t *)((_data)->ptr)
#define REF_TYPE_HASHT(_dt) _dt
#define HAVEBUFF_TYPE_HASHT 0


/** @file hash.h */

/** @ingroup data
 *  @addtogroup hash_t hash_t
 */
/** @ingroup hash_t
 *  @page hash_t_overview Hash overview
 *  
 *  Hashes used to data key-value pairs. This is not crypto nor fast-lookup hashes.
 *  From C point of view, hash is array of hash_t structure. Example of hash:
 *  @code
 *     hash_t somehash[] = {
 *          { HK(key1), DATA_UINTT(100) },      // (1)
 *          { HK(key2), DATA_UINTT(200) },
 *          hash_inline(inlinehash),            // (2)
 *          { HK(key4), DATA_UINTT(400) },
 *          hash_null,                          // (3)
 *          hash_end                            // (4)
 *     };
 *  @endcode
 *  On line marked with (1) we see simple assignment, key1 equals data with type uint_t and value equals to 100.
 *  On line (2) there is inline hash declaration. See @ref hash_t_inline.
 *  Line (3) contain hash_null statement, this declares empty key-value pair and will be ignored on key finds.
 *  This thing is some cases useful for space reservation for further use.
 *  On line (4) there is hash_end keyword indicating hash end. Every hash must end with this statement.
 */ 
/** @ingroup hash_t
 *  @page hash_t_reassign Reassignment
 *  
 *  Because of linear hash_find routine one can define key-value pair earlier than another key-value pair with same key in
 *  hash, this process can be called "reassignment".
 *
 *  This is very useful process, because of:
 *  @li you save old value
 *  @li don't mess with api calls, all process done in current function and very fast.
 *  @li you don't copy data itself from place to place: only pointer.
 *  
 *  Example:
 *  @code
 *      uintmax_t  somenewvalue = 100;
 *
 *      hash_t new_values[] = {
 *         { HK(buffer), DATA_PTR_UINTT(&somenewvalue) },
 *         hash_inline(old_hash),
 *         hash_end
 *      };
 *  @endcode
 */
/** @ingroup hash_t
 *  @page hash_t_inline Inline hashes
 *  
 *  Any hash can have inline hash. This is the way to merge two hashes together and define which hash redefine another's
 *  keys. Example is same as in @ref hash_t_reassign. new_values hash redefine all key-value pairs with key "buffer" in
 *  old_hash. You can have as many levels of inlining as you want, but remember access times and, of course, stack consumption.
 */
/** @ingroup hash_t
 *  @page hash_t_recomendations Recomendations
 *  
 *  @li Don't mess with clear assignment, just redefine. It is cheap and fast.
 *  @li Keep old request inlined in new requests. Hash can contain some keys you can't know about, and some machine is waiting them.
 *  @li To lower access time to hash keep keys than would be accessed many times as top as possible. That's why
 *  action key is at top in most of requests.
 */

/// Hash item
struct hash_t {
	hashkey_t       key;        ///< Hash key 
	data_t          data;       ///< Hash value - data holder
};

typedef enum hash_iter_flags {
	HASH_ITER_NULL   = 1,         ///< Exec callback with hash_null items also
	HASH_ITER_END    = 2,         ///< Exec callback with hash_end items also
	HASH_ITER_INLINE = 4,         ///< Exec callback with hash_inline items also
} hash_iter_flags;

typedef ssize_t  (*hash_iterator)(hash_t *hash, void *arg); ///< Callback routine for hash_iter

#define hash_ptr_null     HK(hash_ptr_null)
#define hash_ptr_end      HK(hash_ptr_end)
#define hash_ptr_inline   HK(hash_ptr_inline)

#define hash_null         {hash_ptr_null,    {0, NULL}}            ///< Mark null item here
#define hash_inline(hash) {hash_ptr_inline,  DATA_PTR_HASHT(hash)} ///< Mark inline hash here
#define hash_next(hash)   {hash_ptr_end,     {0, (void *)hash}} ///< Mark next hash  // TODO deprecate
#define hash_end          hash_next(NULL)    ///< Mark hash end

API hash_t *           hash_new                     (size_t nelements);  ///< Allocate new hash filled with hash_null
API hash_t *           hash_copy                    (hash_t *hash);      ///< Make copy of supplied hash
API void               hash_free                    (hash_t *hash);      ///< Free allocated, or copy of hash
API void               hash_dump                    (hash_t *hash);      ///< Print human-readable version of hash

API hash_t *           hash_find                    (hash_t *hash, hashkey_t key); ///< Find key in hash
API data_t *           hash_data_find               (hash_t *hash, hashkey_t key); ///< Find key in hash and return pointer to data holder
API size_t             hash_nelements               (hash_t *hash); ///< Count number of elements in hash

/** @brief Iterate all hash keys and pass them to callback
 *  @param hash  Hash to iteration
 *  @param func  Callback
 *  @param arg   User-defined argument for callback
 *  @param flags Flags for iteration process
 *  @see hash_iter_flags, hash_iterator
 *  @retval ITER_OK     All items iterated
 *  @retval ITER_BREAK  Callback returned ITER_BREAK and stopped iterations
 */
API ssize_t            hash_iter                    (hash_t *hash, hash_iterator func, void *arg, hash_iter_flags flags);

/** Get value from hash
 * @param _ret  Return value (ssize_t)
 * @param _type Destination data type. Only constants allowed.
 * @param _dt   Destination data.
 * @param _hash Hash to get value from
 * @param _key  Key to search for
 * @see data_get
 */
#define hash_data_get(_ret,_type,_dt,_hash,_key){                  \
	data_get(_ret,                                              \
		_type,                                              \
		_dt,                                                \
		hash_data_find(_hash,_key)                          \
	);                                                          \
	(void)_ret;                                                 \
};

/** Consume value from hash
 * @param _ret  Return value (ssize_t)
 * @param _type Destination data type. Only constants allowed.
 * @param _dt   Destination data.
 * @param _hash Hash to get value from
 * @param _key  Key to search for
 * @see data_get
 */
#define hash_data_consume(_ret,_type,_dt,_hash,_key){               \
	data_consume(_ret,                                          \
		_type,                                              \
		_dt,                                                \
		hash_data_find(_hash,_key)                          \
	);                                                          \
	(void)_ret;                                                 \
};

/** Consume holder from hash
 * @param _ret  Return value (ssize_t)
 * @param _data Destination data holder.
 * @param _hash Hash to get value from
 * @param _key  Key to search for
 * @see data_get
 */
#define hash_holder_consume(_ret,_data,_hash,_key){                 \
	holder_consume(_ret,                                        \
		_data,                                              \
		hash_data_find(_hash,_key)                          \
	);                                                          \
};

/** Convert value from hash.
 * @param _ret  Return value (ssize_t)
 * @param _type Destination data type. Only constants allowed.
 * @param _dt   Destination data.
 * @param _hash Hash to get value from
 * @param _key  Key to search for
 * @see data_get
 */
#define hash_data_convert(_ret,_type,_dt,_hash,_key){               \
	data_convert(_ret,                                          \
		_type,                                              \
		_dt,                                                \
		hash_data_find(_hash,_key)                          \
	);                                                          \
	(void)_ret;                                                 \
};

/** Set value to hash. Use this ONLY in emergency.
 * @param _ret  Return value (ssize_t)
 * @param _type Source data type. Only constants allowed.
 * @param _dt   Source data.
 * @param _hash Hash to set value
 * @param _key  Key to search for
 * @see data_get
 */
#define hash_data_set(_ret,_type,_dt,_hash,_key){                   \
	data_set(_ret,                                              \
		_type,                                              \
		_dt,                                                \
		hash_data_find(_hash,_key)                          \
	);                                                          \
	(void)_ret;                                                 \
};

#define hash_assign_hash_t(_dst, _src) {        \
	memcpy((_dst), (_src), sizeof(hash_t)); \
}
#define hash_assign_hash_inline(_dst, _hash) {  \
	(_dst)->key = hash_ptr_inline;   \
	(_dst)->data.type = TYPE_HASHT;  \
	(_dst)->data.ptr = _hash;        \
}
#define hash_assign_hash_null(_dst) {  \
	(_dst)->key = hash_ptr_null;   \
	(_dst)->data.type = TYPE_VOIDT;\
	(_dst)->data.ptr = NULL;       \
}
#define hash_assign_hash_end(_dst) {  \
	(_dst)->key = hash_ptr_end;   \
	(_dst)->data.ptr = NULL;      \
}

#endif
