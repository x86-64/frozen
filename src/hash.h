#ifndef HASH_H
#define HASH_H

typedef struct hash_t {
	char *          key;
	
	data_type       type;         // DATA_* macros can init this
	void *          value;        //
	size_t          value_size;   //
} hash_t;

#define hash_ptr_null  (void *) 0
#define hash_ptr_end   (void *)-1 

#define hash_null       {hash_ptr_null, 0, hash_ptr_null, 0}
#define hash_next(hash) {hash_ptr_end,  0, (void *)hash,  0}
#define hash_end        hash_next(NULL)

#define HVALUE(_hash_t,_type)  *(_type *)_hash_t->value

API hash_t *           hash_find_value              (hash_t *hash, char *key);
API hash_t *           hash_find_typed_value        (hash_t *hash, data_type type, char *key);
API hash_t *           hash_append_value            (hash_t *hash, data_type type, char *key, void *value, size_t value_size);
API hash_t *           hash_set_value               (hash_t *hash, data_type type, char *key, void *value, size_t value_size);
API void               hash_del_value               (hash_t *hash, char *key);
API void               hash_assign                  (hash_t *hash, hash_t *hash_next);

#ifdef DEBUG
API void               hash_dump                    (hash_t *hash);
#endif

API int                hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
API int                hash_from_buffer             (hash_t **hash, buffer_t *buffer);

#endif // HASH_H
