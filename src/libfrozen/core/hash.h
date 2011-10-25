#ifndef HASH_H
#define HASH_H

/** @file hash.h */

#include <hashkeys.h>

/// Hash item
struct hash_t {
	hash_key_t      key;        ///< Hash key 
	data_t          data;       ///< Hash value - data holder
};

typedef enum hash_iter_flags {
	HASH_ITER_NULL = 1,         ///< Exec callback with hash_null items also
	HASH_ITER_END  = 2          ///< Exec callback with hash_end items also
} hash_iter_flags;

typedef ssize_t  (*hash_iterator)(hash_t *hash, void *arg); ///< Callback routine for hash_iter

#define hash_ptr_null     (hash_key_t)-1
#define hash_ptr_end      (hash_key_t)-2 
#define hash_ptr_inline   (hash_key_t)-3

#define hash_null         {hash_ptr_null,    {0, NULL}}            ///< Mark null item here
#define hash_inline(hash) {hash_ptr_inline,  DATA_PTR_HASHT(hash)} ///< Mark inline hash here
#define hash_next(hash)   {hash_ptr_end,     {0, (void *)hash}} ///< Mark next hash  // TODO deprecate
#define hash_end          hash_next(NULL)    ///< Mark hash end
#define HK(value)         HASH_KEY_##value   ///< Shortcut for registed hash keys. Also used in key registration process.

API hash_t *           hash_new                     (size_t nelements);  ///< Allocate new hash filled with hash_null
API hash_t *           hash_copy                    (hash_t *hash);      ///< Make copy of supplied hash
API void               hash_free                    (hash_t *hash);      ///< Free allocated, or copy of hash

API hash_t *           hash_find                    (hash_t *hash, hash_key_t key); ///< Find key in hash
API data_t *           hash_data_find               (hash_t *hash, hash_key_t key); ///< Find key in hash and return pointer to data holder
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
#define hash_data_copy(_ret,_type,_dt,_hash,_key){                  \
	data_get(_ret,                                              \
		_type,                                              \
		_dt,                                                \
		hash_data_find(_hash,_key)                          \
	);                                                          \
	(void)_ret;                                                 \
};

#define hash_assign_hash_t(_dst, _src) {        \
	memcpy((_dst), (_src), sizeof(hash_t)); \
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

#ifdef DEBUG
API void               hash_dump                    (hash_t *hash); ///< Print human-readable version of hash
#endif

#endif // HASH_H
