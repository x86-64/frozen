#ifndef HASH_H
#define HASH_H

typedef struct hash_el_t {
	unsigned int    type;   // enum data_type
	unsigned int    key;    // char *
	unsigned int    value;  // int / ptr
} hash_el_t;

typedef struct hash_t {
	unsigned int   size;
	unsigned int   nelements;
	
	union {
		unsigned int   is_local;    // local
		unsigned int   reserved1;   // network
	};
	union {
		unsigned int   alloc_size;  // local
		unsigned int   reserved2;   // network
	};
	
	hash_el_t     *elements;
	data_t        *data;
	data_t        *data_end;
} hash_t;

#define HASH_T_LOCAL_SIZE   sizeof(hash_t)
#define HASH_T_NETWORK_SIZE sizeof(unsigned int) * 4

hash_t *           hash_new                     (void);
void               hash_free                    (hash_t *hash);

int                hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
int                hash_from_buffer             (hash_t **hash, buffer_t *buffer);

int                hash_set                     (hash_t *hash, char *key, data_type  type, data_t  *value);
int                hash_get                     (hash_t *hash, char *key, data_type  type, data_t **value, size_t *value_size);
int                hash_get_copy                (hash_t *hash, char *key, data_type  type, data_t  *buf, size_t buf_size);
int                hash_get_any                 (hash_t *hash, char *key, data_type *type, data_t **value, size_t *value_size);

#endif // HASH_H
