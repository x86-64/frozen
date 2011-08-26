#ifndef HASH_H
#define HASH_H

#include <hashkeys.h>

struct hash_t {
	hash_key_t      key;
	data_t          data;
};

typedef ssize_t  (*hash_iterator)(hash_t *, void *, void *);

#define hash_ptr_null  (hash_key_t)-1
#define hash_ptr_end   (hash_key_t)-2 

#define hash_null       {hash_ptr_null, {0, NULL, 0}}
#define hash_next(hash) {hash_ptr_end,  {0, (void *)hash,  0}}
#define hash_end        hash_next(NULL)
#define HASH_CTX_KEY_OFFSET 0x1000
#define HK(value) HASH_KEY_##value

// allocated hash functions
API hash_t *           hash_new                     (size_t nelements);
API hash_t *           hash_copy                    (hash_t *hash);
API void               hash_free                    (hash_t *hash);

// general functions
API hash_t *           hash_find                    (hash_t *hash, hash_key_t key);
API ssize_t            hash_iter                    (hash_t *hash, hash_iterator func, void *arg1, void *arg2);
API void               hash_chain                   (hash_t *hash, hash_t *hash_next);
API void               hash_unchain                 (hash_t *hash, hash_t *hash_unchain);
API size_t             hash_nelements               (hash_t *hash);

// hash <=> buffer
API ssize_t            hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
API ssize_t            hash_from_buffer             (hash_t **hash, buffer_t *buffer);

// hash <=> raw memory
API ssize_t            hash_to_memory               (hash_t  *hash, void *memory, size_t memory_size);
API ssize_t            hash_reread_from_memory      (hash_t  *hash, void *memory, size_t memory_size);
API ssize_t            hash_from_memory             (hash_t **hash, void *memory, size_t memory_size);

// hash keys routines
API hash_key_t         hash_string_to_key           (char *string);
API char *             hash_key_to_string           (hash_key_t key);
API hash_key_t         hash_key_to_ctx_key          (hash_key_t key);

_inline
API hash_key_t         hash_item_key                (hash_t *hash){ return hash->key; }
_inline
API size_t             hash_item_is_null            (hash_t *hash){ return (hash->key == hash_ptr_null); }
_inline
API data_t *           hash_item_data               (hash_t *hash){ return &(hash->data); }
_inline
API hash_t *           hash_item_next               (hash_t *hash){ return ((hash + 1)->key == hash_ptr_end) ? NULL : hash + 1; }
_inline
API void               hash_data_find               (hash_t *hash, hash_key_t key, data_t **data, data_ctx_t **data_ctx){
	hash_t *temp;
	if(data){
		*data     =
			((temp = hash_find(hash, key                     )) == NULL) ?
			NULL : hash_item_data(temp);
	}
	if(data_ctx){
		*data_ctx =
			((temp = hash_find(hash, hash_key_to_ctx_key(key))) == NULL) ?
			NULL : (data_ctx_t *)hash_item_data(temp);
	}
}

#define hash_data_copy(_ret,_type,_dt,_hash,_key){                  \
	hash_t *__temp;                                             \
	if( (__temp = hash_find(_hash,_key)) != NULL){              \
		data_to_dt(_ret,_type,_dt,(&(__temp->data)),NULL);  \
	}else{ _ret = -EINVAL; }                                    \
	(void)_ret; \
}

#define hash_assign_hash_t(_dst, _src) {        \
	memcpy((_dst), (_src), sizeof(hash_t)); \
}
#define hash_assign_hash_null(_dst) {  \
	(_dst)->key = hash_ptr_null;   \
	(_dst)->data.type = TYPE_VOIDT; \
	(_dst)->data.data_ptr = NULL;  \
	(_dst)->data.data_size = 0;    \
}
#define hash_assign_hash_end(_dst) {  \
	(_dst)->key = hash_ptr_end;   \
	(_dst)->data.data_ptr = NULL; \
}


#ifdef DEBUG

API void               hash_dump                    (hash_t *hash);

#endif
#endif // HASH_H
