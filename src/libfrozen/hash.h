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

#define HVALUE(_hash_t,_type)  *(_type *)_hash_t->data.data_ptr

#define hash_assign_hash_t(_dst, _src) {        \
	memcpy((_dst), (_src), sizeof(hash_t)); \
}
#define hash_assign_hash_null(_dst) { \
	(_dst)->key = hash_ptr_null;  \
}
#define hash_assign_hash_end(_dst) {  \
	(_dst)->key = hash_ptr_end;   \
	(_dst)->data.data_ptr = NULL; \
}

// TODO deprecate this
API int                hash_get                     (hash_t *hash, hash_key_t key, data_type *type, void **value, size_t *value_size);
API int                hash_get_typed               (hash_t *hash, data_type type, hash_key_t key, void **value, size_t *value_size);
API hash_t *           hash_set                     (hash_t *hash, hash_key_t key, data_type type, void *value, size_t value_size);
API void               hash_delete                  (hash_t *hash, hash_key_t key);
// END TODO

API void               hash_chain                   (hash_t *hash, hash_t *hash_next);
API ssize_t            hash_iter                    (hash_t *hash, hash_iterator func, void *arg1, void *arg2);
API hash_t *           hash_find                    (hash_t *hash, hash_key_t key);
API hash_t *           hash_find_typed              (hash_t *hash, data_type type, hash_key_t key);

API data_t *           hash_get_data                (hash_t *hash, hash_key_t key);
API data_t *           hash_get_typed_data          (hash_t *hash, data_type type, hash_key_t key);
API data_ctx_t *       hash_get_data_ctx            (hash_t *hash, hash_key_t key);

API data_type          hash_get_data_type           (hash_t *hash); // TODO deprecate
API void *             hash_get_value_ptr           (hash_t *hash);
API size_t             hash_get_value_size          (hash_t *hash);

API hash_key_t         hash_string_to_key           (char *string);
API char *             hash_key_to_string           (hash_key_t key);

//API ssize_t            hash_copy_data               (data_type type, void *dt, hash_t *hash, hash_key_t key);

API hash_t *           hash_copy                    (hash_t *hash);
API void               hash_free                    (hash_t *hash);
API size_t             hash_get_nelements           (hash_t *hash);

#ifdef DEBUG
API void               hash_dump                    (hash_t *hash);
#endif

API int                hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
API int                hash_from_buffer             (hash_t **hash, buffer_t *buffer);

#define hash_copy_data(_ret,_type,_dt,_hash,_key){       \
	data_t *temp;                                    \
	if( (temp = hash_get_data(_hash,_key)) != NULL){ \
		data_to_dt(_ret,_type,_dt,temp,NULL);    \
	}                                                \
}

#endif // HASH_H
