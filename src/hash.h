#ifndef HASH_H
#define HASH_H

typedef struct hash_t {
	data_type       type;
	char *          key;
	void *          value;
} hash_t;

#define hash_ptr_null  (void *) 0
#define hash_ptr_end   (void *)-1 

#define hash_null {0, hash_ptr_null, hash_ptr_null}
#define hash_end  {0, hash_ptr_end,  hash_ptr_end }

#define HVALUE(_hash_t,_type)  *(_type *)_hash_t->value

API hash_t *           hash_find_value              (hash_t *hash, char *key);
API hash_t *           hash_find_typed_value        (hash_t *hash, data_type type, char *key);
API hash_t *           hash_add_value               (hash_t *hash, data_type type, char *key, void *value);
API void               hash_del_value               (hash_t *hash, char *key);

#ifdef DEBUG
API void               hash_dump                    (hash_t *hash);
#endif

API int                hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
API int                hash_from_buffer             (hash_t **hash, buffer_t *buffer);

#endif // HASH_H
